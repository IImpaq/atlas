//
// Created by Marcus Gugacs on 28.10.24.
//

#ifndef ATLAS_PACKAGE_INSTALLER_HPP
#define ATLAS_PACKAGE_INSTALLER_HPP

#include <filesystem>
#include <regex>

#include <json/json.h>

#include "pods/PackageConfig.hpp"

namespace fs = std::filesystem;

class PackageInstaller {
private:
    fs::path m_cache_dir;
    fs::path m_install_dir;
    fs::path m_log_dir;
    Json::Value m_config;
    std::string m_platform;

public:
    PackageInstaller(const fs::path &a_cache, const fs::path &a_install,
                     const fs::path &a_log, const PackageConfig &a_package_config);

    bool Download();
    bool Prepare();
    bool Build();
    bool Install();
    bool Cleanup();
    bool Uninstall();

private:
    bool executeCommands(const Json::Value &a_commands);
    std::string replaceVariables(const std::string &a_cmd);
    bool downloadFile(const std::string &a_url, const std::string &a_target);
};

#endif // ATLAS_PACKAGE_INSTALLER_HPP
