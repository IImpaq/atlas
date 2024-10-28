//
// Created by Marcus Gugacs on 28.10.24.
//

#include "PackageInstaller.hpp"

#include <curl/curl.h>

#include "utils/Misc.hpp"

PackageInstaller::PackageInstaller(const fs::path &a_cache, const fs::path &a_install, const fs::path &a_log,
  const PackageConfig &a_package_config): m_cache_dir(a_cache), m_install_dir(a_install), m_log_dir(a_log) {
#ifdef __APPLE__
  m_platform = "macos";
#else
        platform = "linux";
#endif

  // Load package.json from the package's directory
  fs::path packageJsonPath = m_cache_dir / a_package_config.repository /
                             "packages" / a_package_config.name / "package.json";
  std::ifstream configFile(packageJsonPath);
  if (configFile.is_open()) {
    configFile >> m_config;
  }
}

bool PackageInstaller::Download() {
  const auto &step = m_config["platforms"][m_platform]["steps"]["download"];
  return downloadFile(step["url"].asString(), step["target"].asString());
}

bool PackageInstaller::Prepare() {
  return executeCommands(
    m_config["platforms"][m_platform]["steps"]["prepare"]["commands"]);
}

bool PackageInstaller::Build() {
  return executeCommands(
    m_config["platforms"][m_platform]["steps"]["build"]["commands"]);
}

bool PackageInstaller::Install() {
  return executeCommands(
    m_config["platforms"][m_platform]["steps"]["install"]["commands"]);
}

bool PackageInstaller::Cleanup() {
  return executeCommands(
    m_config["platforms"][m_platform]["steps"]["cleanup"]["commands"]);
}

bool PackageInstaller::Uninstall() {
  return executeCommands(
    m_config["platforms"][m_platform]["steps"]["uninstall"]["commands"]);
}

bool PackageInstaller::executeCommands(const Json::Value &a_commands) {
  for (const auto &cmd : a_commands) {
    std::string command = replaceVariables(cmd.asString());
    if (ProcessCommand(command, m_log_dir / "latest.log", false) != 0) {
      return false;
    }
  }
  return true;
}

std::string PackageInstaller::replaceVariables(const std::string &a_cmd) {
  std::string result = a_cmd;
  result = std::regex_replace(result, std::regex("\\$PACKAGE_CACHE_DIR"),
                              m_cache_dir.string());
  result = std::regex_replace(result, std::regex("\\$INSTALL_DIR"),
                              m_install_dir.string());
  return result;
}

bool PackageInstaller::downloadFile(const std::string &a_url, const std::string &a_target) {
  std::string targetPath = replaceVariables(a_target);
  CURL *curl = curl_easy_init();
  if (!curl)
    return false;

  FILE *fp = fopen(targetPath.c_str(), "wb");
  if (!fp) {
    curl_easy_cleanup(curl);
    return false;
  }

  curl_easy_setopt(curl, CURLOPT_URL, a_url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  CURLcode res = curl_easy_perform(curl);
  fclose(fp);
  curl_easy_cleanup(curl);

  return res == CURLE_OK;
}
