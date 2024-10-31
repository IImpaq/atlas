//
// Created by Marcus Gugacs on 28.10.24.
//

#ifndef ATLAS_PACKAGE_INSTALLER_HPP
#define ATLAS_PACKAGE_INSTALLER_HPP

#include <filesystem>
#include <regex>

#include <data/String.hpp>
#include <json/json.h>

#include "pods/PackageConfig.hpp"

namespace fs = std::filesystem;

class PackageInstaller {
private:
    fs::path m_cache_dir;
    fs::path m_install_dir;
    fs::path m_log_dir;
    Json::Value m_config;
    ntl::String m_platform;

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
    ntl::String replaceVariables(const ntl::String &a_cmd);
    bool downloadFile(const ntl::String &a_url, const ntl::String &a_target);
};

#endif // ATLAS_PACKAGE_INSTALLER_HPP
