//
// Created by Marcus Gugacs on 28.10.24.
//

#ifndef PACKAGE_PARSER_HPP
#define PACKAGE_PARSER_HPP


#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

using json = nlohmann::json;

class PackageParser {
private:
    std::string url;
    std::unordered_map<std::string, std::vector<std::string>> array_variables;
    std::unordered_map<std::string, std::string> variables;

public:
    PackageParser(const std::string& url);

    json parse();

private:
    std::string fetch_pkgbuild(const std::string &url);
    std::string extract_function_content(const std::string& pkgbuild, const std::string& func_name);
    std::vector<std::string> parse_function_commands(const std::string& content);
    std::string trim(const std::string& str);
    std::string resolve_variables(const std::string& input);
    std::vector<std::string> extract_function_body(const std::string& pkgbuild, const std::string& func_name);

    void parse_arrays(const std::string& pkgbuild);
    void parse_variables(const std::string& pkgbuild);

    json create_linux_steps(const std::string& pkgbuild);
    json create_macos_steps(const json& linux_steps);

    json process_json_values(json& j, const std::string& pkg_name, const std::string& pkg_version);
    json parse_pkgbuild(const std::string& pkgbuild_content);
};

#endif // PACKAGE_PARSER_HPP
