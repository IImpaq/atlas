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
    std::string m_url;
    std::unordered_map<std::string, std::vector<std::string>> m_array_variables;
    std::unordered_map<std::string, std::string> m_variables;

public:
    explicit PackageParser(const std::string& a_url);

    json Parse();

private:
    std::string fetchPkgbuild(const std::string &a_url);
    std::string extractFunctionContent(const std::string& a_pkgbuild, const std::string& a_func_name);
    std::vector<std::string> parseFunctionCommands(const std::string& a_content);
    std::string trim(const std::string& str);
    std::string resolveVariables(const std::string& a_input);
    std::vector<std::string> extractFunctionBody(const std::string& a_pkgbuild, const std::string& a_func_name);

    void parseArrays(const std::string& a_pkgbuild);
    void parseVariables(const std::string& a_pkgbuild);

    json createLinuxSteps(const std::string& a_pkgbuild);
    json createMacosSteps(const json& a_linux_steps);

    json processJsonValues(json& j, const std::string& a_pkg_name, const std::string& a_pkg_version);
    json parsePkgbuild(const std::string& a_pkgbuild_content);
};

#endif // PACKAGE_PARSER_HPP
