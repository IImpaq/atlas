/**
* @file PackageInstaller.cpp
* @author Marcus Gugacs
* @date 28.10.24
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#include "PackageInstaller.hpp"

#include <curl/curl.h>

#include "utils/Misc.hpp"

namespace atlas {
  PackageInstaller::PackageInstaller(const fs::path& a_cache, const fs::path& a_install, const fs::path& a_log,
                                     const PackageConfig& a_package_config): m_cache_dir(a_cache),
                                                                             m_install_dir(a_install),
                                                                             m_log_dir(a_log) {
#ifdef __APPLE__
    m_platform = "macos";
#else
          platform = "linux";
#endif

    // Load package.json from the package's directory
    fs::path packageJsonPath = m_cache_dir / a_package_config.repository.GetCString() /
                               "packages" / a_package_config.name.GetCString() / "package.json";
    std::ifstream configFile(packageJsonPath);
    if (configFile.is_open()) {
      configFile >> m_config;
    }
  }

  bool PackageInstaller::Download() {
    const auto& step = m_config["platforms"][m_platform.GetCString()]["steps"]["download"];
    return downloadFile(step["url"].asString().c_str(), step["target"].asString().c_str());
  }

  bool PackageInstaller::Prepare() {
    return executeCommands(
      m_config["platforms"][m_platform.GetCString()]["steps"]["prepare"]["commands"]);
  }

  bool PackageInstaller::Build() {
    return executeCommands(
      m_config["platforms"][m_platform.GetCString()]["steps"]["build"]["commands"]);
  }

  bool PackageInstaller::Install() {
    return executeCommands(
      m_config["platforms"][m_platform.GetCString()]["steps"]["install"]["commands"]);
  }

  bool PackageInstaller::Cleanup() {
    return executeCommands(
      m_config["platforms"][m_platform.GetCString()]["steps"]["cleanup"]["commands"]);
  }

  bool PackageInstaller::Uninstall() {
    return executeCommands(
      m_config["platforms"][m_platform.GetCString()]["steps"]["uninstall"]["commands"]);
  }

  bool PackageInstaller::executeCommands(const Json::Value& a_commands) {
    for (const auto& cmd : a_commands) {
      ntl::String command = replaceVariables(cmd.asString().c_str());
      if (ProcessCommand(command, ntl::String{(m_log_dir / "latest.log").c_str()}, false) != 0) {
        return false;
      }
    }
    return true;
  }

  ntl::String PackageInstaller::replaceVariables(const ntl::String& a_cmd) {
    ntl::String result = a_cmd;
    result = std::regex_replace(result.GetCString(), std::regex("\\$PACKAGE_CACHE_DIR"), m_cache_dir.string()).c_str();
    result = std::regex_replace(result.GetCString(), std::regex("\\$INSTALL_DIR"), m_install_dir.string()).c_str();
    return result;
  }

  bool PackageInstaller::downloadFile(const ntl::String& a_url, const ntl::String& a_target) {
    ntl::String targetPath = replaceVariables(a_target);
    CURL* curl = curl_easy_init();
    if (!curl)
      return false;

    FILE* fp = fopen(targetPath.GetCString(), "wb");
    if (!fp) {
      curl_easy_cleanup(curl);
      return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, a_url.GetCString());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);

    return res == CURLE_OK;
  }
}
