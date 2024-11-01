/**
* @file Atlas.hpp
* @author Marcus Gugacs
* @date 28.10.24
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#ifndef ATLAS_ATLAS_HPP
#define ATLAS_ATLAS_HPP

#include <cstdlib>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <json/json.h>
#include <string>

#include <data/Array.hpp>
#include <data/String.hpp>
#include <data/Map.hpp>
#include <os/Lock.hpp>

#include "core/Config.hpp"
#include "core/PackageInstaller.hpp"
#include "pods/FetchData.hpp"
#include "pods/PackageConfig.hpp"
#include "pods/Repository.hpp"
#include "pods/InstallerData.hpp"
#include "utils/LoadingAnimation.hpp"
#include "utils/MultiLoadingAnimation.hpp"

namespace fs = std::filesystem;

namespace atlas {
  class Atlas {
  private:
    Config m_config;

    const fs::path m_install_dir;
    const fs::path m_cache_dir;
    const fs::path m_shortcut_dir;
    const fs::path m_repo_config_path;
    const fs::path m_log_dir;

    MultiLoadingAnimation m_animator;

    ntl::Map<ntl::String, Repository>     m_repositories;
    ntl::SharedLock                       m_repositories_lock;

    ntl::Map<ntl::String, PackageConfig>  m_package_index;
    ntl::SharedLock                       m_package_index_lock;

    FetchData       m_fetch_data;
    ntl::SharedLock m_fetch_data_lock;

    InstallerData   m_installer_data;
    ntl::SharedLock m_installer_data_lock;

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
