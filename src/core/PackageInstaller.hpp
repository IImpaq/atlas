//
// Created by Marcus Gugacs on 28.10.24.
//

#ifndef ATLAS_PACKAGE_INSTALLER_HPP
#define ATLAS_PACKAGE_INSTALLER_HPP

#include <filesystem>
#include <regex>

#include <json/json.h>
#include <curl/curl.h>

#include "utils/Misc.hpp"
#include "pods/PackageConfig.hpp"

namespace fs = std::filesystem;

class PackageInstaller {
private:
    fs::path cacheDir;
    fs::path installDir;
    fs::path logDir;
    Json::Value config;
    std::string platform;

    bool executeCommands(const Json::Value &commands) {
        for (const auto &cmd : commands) {
            std::string command = replaceVariables(cmd.asString());
            if (processCommand(command, logDir / "latest.log", false) != 0) {
                return false;
            }
        }
        return true;
    }

    std::string replaceVariables(const std::string &cmd) {
        std::string result = cmd;
        result = std::regex_replace(result, std::regex("\\$PACKAGE_CACHE_DIR"),
                                    cacheDir.string());
        result = std::regex_replace(result, std::regex("\\$INSTALL_DIR"),
                                    installDir.string());
        return result;
    }

    bool downloadFile(const std::string &url, const std::string &target) {
        std::string targetPath = replaceVariables(target);
        CURL *curl = curl_easy_init();
        if (!curl)
            return false;

        FILE *fp = fopen(targetPath.c_str(), "wb");
        if (!fp) {
            curl_easy_cleanup(curl);
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        CURLcode res = curl_easy_perform(curl);
        fclose(fp);
        curl_easy_cleanup(curl);

        return res == CURLE_OK;
    }

public:
    PackageInstaller(const fs::path &cache, const fs::path &install,
                     const fs::path &log, const PackageConfig &packageConfig)
            : cacheDir(cache), installDir(install), logDir(log) {
#ifdef __APPLE__
        platform = "macos";
#else
        platform = "linux";
#endif

        // Load package.json from the package's directory
        fs::path packageJsonPath = cacheDir / packageConfig.repository /
                                   "packages" / packageConfig.name / "package.json";
        std::ifstream configFile(packageJsonPath);
        if (configFile.is_open()) {
            configFile >> config;
        }
    }

    bool download() {
        const auto &step = config["platforms"][platform]["steps"]["download"];
        return downloadFile(step["url"].asString(), step["target"].asString());
    }

    bool prepare() {
        return executeCommands(
                config["platforms"][platform]["steps"]["prepare"]["commands"]);
    }

    bool build() {
        return executeCommands(
                config["platforms"][platform]["steps"]["build"]["commands"]);
    }

    bool install() {
        return executeCommands(
                config["platforms"][platform]["steps"]["install"]["commands"]);
    }

    bool cleanup() {
        return executeCommands(
                config["platforms"][platform]["steps"]["cleanup"]["commands"]);
    }

    bool uninstall() {
        return executeCommands(
                config["platforms"][platform]["steps"]["uninstall"]["commands"]);
    }
};

#endif // ATLAS_PACKAGE_INSTALLER_HPP
