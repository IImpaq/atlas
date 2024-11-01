//
// Created by Marcus Gugacs on 28.10.24.
//

#ifndef ATLAS_ATLAS_HPP
#define ATLAS_ATLAS_HPP

#include <cstdlib>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <json/json.h>
#include <string>

#include "Config.hpp"
#include "pods/PackageConfig.hpp"
#include "pods/Repository.hpp"
#include "utils/LoadingAnimation.hpp"
#include "core/PackageInstaller.hpp"

#include <data/Array.hpp>
#include <data/String.hpp>
#include <os/Lock.hpp>

#include "data/Map.hpp"
#include "utils/MultiLoadingAnimation.hpp"

namespace fs = std::filesystem;

namespace atlas {
  class Atlas {
  private:
    Config m_config;

    fs::path m_install_dir;
    fs::path m_cache_dir;
    fs::path m_shortcut_dir;
    fs::path m_repo_config_path;
    fs::path m_log_dir;
    ntl::Map<ntl::String, Repository> m_repositories;
    ntl::Map<ntl::String, PackageConfig> m_package_index;

    ntl::Array<ntl::String> successful_installs;
    ntl::Array<ntl::String> failed_installs;
    ntl::Array<PackageConfig> configs;
    ntl::Map<ntl::String, bool> scheduled;
    std::atomic<bool> any_failure{false};
    MultiLoadingAnimation loading;
    ntl::Lock mutex;

  public:
    Atlas(const fs::path &a_install, const fs::path &a_cache, bool a_verbose);
    ~Atlas();

    bool AddRepository(const ntl::String &a_name, const ntl::String &a_url, const ntl::String &a_branch = "main");

    bool RemoveRepository(const ntl::String &a_name);

    bool EnableRepository(const ntl::String &a_name);

    bool DisableRepository(const ntl::String &a_name);

    void ListRepositories();

    bool Fetch();

    bool Install(const ntl::Array<ntl::String>& a_package_names);

    bool Install(const ntl::String &a_package_name);

    bool Remove(const ntl::String &a_package_name);

    bool Update();

    bool Upgrade(const ntl::String &a_package_name);

    bool LockPackage(const ntl::String& name);

    bool UnlockPackage(const ntl::String& name);

    void Cleanup();

    bool KeepPackage(const ntl::String& name);

    bool UnkeepPackage(const ntl::String& name);

    std::vector<ntl::String> Search(const ntl::String &a_query);

    void Info(const ntl::String &a_package_name);

    bool IsInstalled(const ntl::String &a_package_name) const;

    bool AtlasSetup();

    bool AtlasPurge();

  private:
    void loadRepositories();

    void saveRepositories();

    void loadPackageIndex();

    bool fetchRepository(const Repository &a_repo) const;

    bool installPackage(const PackageConfig &a_config);

    bool removePackage(const PackageConfig &a_config);

    void recordInstallation(const PackageConfig &a_config);

    void recordRemoval(const PackageConfig &a_config);

    bool upgrade(const PackageConfig &a_config);

    ntl::String getCurrentDateTime();

    bool isMacOS();

    bool downloadRepository(const ntl::String &a_username, const ntl::String &a_repo) const;

    bool extractPackage(const ntl::String &a_repo) const;

    void cleanupPackages();
  };
}

#endif // ATLAS_ATLAS_HPP
