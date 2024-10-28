//
// Created by Marcus Gugacs on 28.10.24.
//

#include "PackageParser.hpp"

#include <iostream>
#include <regex>
#include <sstream>
#include <curl/curl.h>

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp) {
  userp->append((char*)contents, size * nmemb);
  return size * nmemb;
}

PackageParser::PackageParser(const std::string &url): url(url) {}

json PackageParser::parse() {
  std::string pkgbuild_content = fetch_pkgbuild(url);
  json result = parse_pkgbuild(pkgbuild_content);
  return result;
}

std::string PackageParser::fetch_pkgbuild(const std::string &url) {
  CURL* curl = curl_easy_init();
  std::string buffer;

  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
  }
  return buffer;
}

std::string PackageParser::extract_function_content(const std::string &pkgbuild, const std::string &func_name) {
  size_t pos = pkgbuild.find(func_name + "()");
  if (pos == std::string::npos) return "";

  pos = pkgbuild.find('{', pos);
  if (pos == std::string::npos) return "";

  int brace_count = 1;
  size_t start = pos + 1;
  size_t current = start;

  while (current < pkgbuild.length() && brace_count > 0) {
    if (pkgbuild[current] == '{') brace_count++;
    if (pkgbuild[current] == '}') brace_count--;
    current++;
  }

  if (brace_count != 0) return "";
  return pkgbuild.substr(start, current - start - 1);
}

std::vector<std::string> PackageParser::parse_function_commands(const std::string &content) {
  std::vector<std::string> commands;
  std::string current_command;
  bool in_quote = false;
  char quote_char = 0;

  for (size_t i = 0; i < content.length(); i++) {
    char c = content[i];

    if (c == '"' || c == '\'') {
      if (!in_quote) {
        in_quote = true;
        quote_char = c;
      } else if (c == quote_char) {
        in_quote = false;
      }
    }

    if (c == '\n' && !in_quote) {
      if (!current_command.empty()) {
        current_command = trim(current_command);
        if (!current_command.empty() && current_command[0] != '#') {
          commands.push_back(current_command);
        }
        current_command.clear();
      }
    } else {
      current_command += c;
    }
  }

  if (!current_command.empty()) {
    current_command = trim(current_command);
    if (!current_command.empty() && current_command[0] != '#') {
      commands.push_back(current_command);
    }
  }

  return commands;
}

std::string PackageParser::trim(const std::string &str) {
  size_t first = str.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return "";
  size_t last = str.find_last_not_of(" \t\r\n");
  return str.substr(first, last - first + 1);
}

std::string PackageParser::resolve_variables(const std::string &input) {
  std::string result = input;
  std::regex var_pattern("\\$\\{([A-Za-z0-9_]+)\\}|\\$([A-Za-z0-9_]+)");

  std::smatch matches;
  while (std::regex_search(result, matches, var_pattern)) {
    std::string var_name = matches[1].str().empty() ? matches[2].str() : matches[1].str();
    if (variables.find(var_name) != variables.end()) {
      result.replace(matches.position(), matches.length(), variables[var_name]);
    }
  }
  return result;
}

std::vector<std::string> PackageParser::
extract_function_body(const std::string &pkgbuild, const std::string &func_name) {
  std::vector<std::string> commands;
  std::regex function_pattern(func_name + "\\s*\\(\\)\\s*\\{([^}]+)\\}");
  std::smatch matches;

  if (std::regex_search(pkgbuild, matches, function_pattern)) {
    std::string body = matches[1];
    std::stringstream ss(body);
    std::string line;

    while (std::getline(ss, line)) {
      // Trim whitespace
      line = std::regex_replace(line, std::regex("^\\s+|\\s+$"), "");

      // Skip empty lines and comments
      if (line.empty() || line[0] == '#') {
        continue;
      }

      // Skip the function opening/closing braces
      if (line == "{" || line == "}") {
        continue;
      }

      // Resolve variables in the command
      line = resolve_variables(line);

      commands.push_back(line);
    }
  }

  return commands;
}

void PackageParser::parse_arrays(const std::string &pkgbuild) {
  std::regex array_pattern("([A-Za-z0-9_]+)=\\(([^)]+)\\)");
  auto begin = std::sregex_iterator(pkgbuild.begin(), pkgbuild.end(), array_pattern);
  auto end = std::sregex_iterator();

  for (std::sregex_iterator i = begin; i != end; ++i) {
    std::smatch match = *i;
    std::string array_content = match[2].str();
    std::vector<std::string> elements;

    std::regex element_pattern("'([^']+)'|\"([^\"]+)\"|([^\\s'\"]+)");
    auto element_begin = std::sregex_iterator(array_content.begin(), array_content.end(), element_pattern);

    for (auto j = element_begin; j != end; ++j) {
      std::smatch element_match = *j;
      std::string element = element_match[1].str();
      if (element.empty()) element = element_match[2].str();
      if (element.empty()) element = element_match[3].str();
      if (!element.empty()) {
        elements.push_back(element);
      }
    }

    array_variables[match[1].str()] = elements;
  }
}

void PackageParser::parse_variables(const std::string &pkgbuild) {
  // Parse basic variables
  std::regex assignment("([A-Za-z0-9_]+)=([\"']?)([^\"'\\n]+)\\2");
  auto begin = std::sregex_iterator(pkgbuild.begin(), pkgbuild.end(), assignment);
  auto end = std::sregex_iterator();

  for (std::sregex_iterator i = begin; i != end; ++i) {
    std::smatch match = *i;
    variables[match[1]] = match[3];
  }

  // Parse arrays
  parse_arrays(pkgbuild);
}

json PackageParser::create_linux_steps(const std::string &pkgbuild) {
  json steps;

  // Download step
  if (variables.find("source") != variables.end()) {
    steps["download"] = {
      {"url", resolve_variables(array_variables["source"][0])},
      {"target", "${PACKAGE_CACHE_DIR}/" + variables["pkgname"] + "-" + variables["pkgver"] + ".tar.gz"}
    };
  }

  std::string prepare_content = extract_function_content(pkgbuild, "prepare");
  std::string build_content = extract_function_content(pkgbuild, "build");
  std::string package_content = extract_function_content(pkgbuild, "package");

  steps["prepare"]["commands"] = parse_function_commands(prepare_content);
  steps["build"]["commands"] = parse_function_commands(build_content);
  steps["install"]["commands"] = parse_function_commands(package_content);

  // Cleanup step
  steps["cleanup"]["commands"] = {
    "rm -rf ${PACKAGE_CACHE_DIR}/" + variables["pkgname"] + "-" + variables["pkgver"],
    "rm -f ${PACKAGE_CACHE_DIR}/" + variables["pkgname"] + "-" + variables["pkgver"] + ".tar.gz"
  };

  // Uninstall step
  steps["uninstall"]["commands"] = {
    "rm -rf /usr/bin/" + variables["pkgname"],
    "rm -rf /usr/lib/" + variables["pkgname"],
    "rm -rf /usr/share/" + variables["pkgname"]
  };

  return steps;
}

json PackageParser::create_macos_steps(const json &linux_steps) {
  json macos_steps = linux_steps;

  // Convert all paths from /usr to /usr/local for macOS
  for (auto& step : macos_steps.items()) {
    if (step.value().contains("commands")) {
      for (auto& command : step.value()["commands"]) {
        std::string cmd = command.get<std::string>();
        cmd = std::regex_replace(cmd, std::regex("/usr(?!/local)"), "/usr/local");
        command = cmd;
      }
    }
  }

  return macos_steps;
}

json PackageParser::process_json_values(json &j, const std::string &pkg_name, const std::string &pkg_version) {
  if (j.is_string()) {
    std::string value = j.get<std::string>();
    value = std::regex_replace(value, std::regex("\\$\\{pkgname\\}"), pkg_name);
    value = std::regex_replace(value, std::regex("\\$\\{pkgver\\}"), pkg_version);
    value = std::regex_replace(value, std::regex("\\$\\{pkgdir\\}"), "/tmp/pkg");
    j = value;
  } else if (j.is_object()) {
    for (auto& [key, value] : j.items()) {
      process_json_values(value, pkg_name, pkg_version);
    }
  } else if (j.is_array()) {
    for (auto& element : j) {
      process_json_values(element, pkg_name, pkg_version);
    }
  }
  return j;
}

json PackageParser::parse_pkgbuild(const std::string &pkgbuild_content) {
  parse_variables(pkgbuild_content);

  json package_json;

  std::string pkg_name = variables["pkgname"];
  std::string pkg_version = variables["pkgver"];

  // Basic metadata
  package_json["name"] = pkg_name;
  package_json["version"] = pkg_version;
  package_json["description"] = variables["pkgdesc"];

  // Platform-specific steps
  json platforms;

  // Linux steps
  platforms["linux"]["steps"] = create_linux_steps(pkgbuild_content);

  // Convert Linux steps to macOS steps
  platforms["macos"]["steps"] = create_macos_steps(platforms["linux"]["steps"]);

  package_json["platforms"] = platforms;

  // Dependencies
  std::vector<std::string> deps;
  if (array_variables.find("depends") != array_variables.end()) {
    deps = array_variables["depends"];
  }
  package_json["dependencies"] = deps;

  // Process and replace all variables in the JSON
  process_json_values(package_json, pkg_name, pkg_version);

  return package_json;
}
