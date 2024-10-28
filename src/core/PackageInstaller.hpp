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
    fs::path cacheDir;
    fs::path installDir;
    fs::path logDir;
    Json::Value config;
    std::string platform;

public:
    PackageInstaller(const fs::path &cache, const fs::path &install,
                     const fs::path &log, const PackageConfig &packageConfig);

    bool download();
    bool prepare();
    bool build();
    bool install();
    bool cleanup();
    bool uninstall();

private:
    bool executeCommands(const Json::Value &commands);
    std::string replaceVariables(const std::string &cmd);
    bool downloadFile(const std::string &url, const std::string &target);
};

#endif // ATLAS_PACKAGE_INSTALLER_HPP
