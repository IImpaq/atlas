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
  /**
   * @class Atlas
   * @brief The main class of the atlas package manager.
   */
  class Atlas {
  private:
    Config m_config;

    const fs::path m_install_dir;
    const fs::path m_cache_dir;
    const fs::path m_shortcut_dir;
    const fs::path m_repo_config_path;
    const fs::path m_log_dir;

    MultiLoadingAnimation m_animator;

    ntl::Map<ntl::String, Repository> m_repositories;
    ntl::SharedLock m_repositories_lock;

    ntl::Map<ntl::String, PackageConfig> m_package_index;
    ntl::SharedLock m_package_index_lock;

    FetchData m_fetch_data;
    ntl::SharedLock m_fetch_data_lock;

    InstallerData m_installer_data;
    ntl::SharedLock m_installer_data_lock;

  public:
    /**
     * @brief Constructor for the Atlas class.
     *
     * Initializes the install directory, cache directory, and shortcut directory.
     * Also initializes the configuration object.
     *
     * @param a_install Install directory path
     * @param a_cache Cache directory path
     * @param a_verbose Whether to enable verbose mode (default: false)
     */
    Atlas(const fs::path& a_install, const fs::path& a_cache, bool a_verbose);

    /**
     * @brief Destructor for the Atlas class.
     */
    ~Atlas();

    /**
     * @brief Adds a new repository to the atlas package manager.
     *
     * This method creates a new Repository object and adds it to the internal map of repositories.
     *
     * @param a_name Name of the repository
     * @param a_url URL of the repository
     * @param a_branch Branch of the repository (default: "main")
     * @return Whether the operation was successful
     */
    bool AddRepository(const ntl::String& a_name, const ntl::String& a_url, const ntl::String& a_branch = "main");

    /**
     * @brief Removes a repository from the atlas package manager.
     *
     * This method removes the specified repository from the internal map of repositories.
     *
     * @param a_name Name of the repository to remove
     * @return Whether the operation was successful
     */
    bool RemoveRepository(const ntl::String& a_name);

    /**
     * @brief Enables or disables a repository in the atlas package manager.
     *
     * This method updates the status of the specified repository in the internal map of repositories.
     *
     * @param a_name Name of the repository to enable/disable
     * @return Whether the operation was successful
     */
    bool EnableRepository(const ntl::String& a_name);

    /**
     * @brief Disables a repository in the atlas package manager.
     *
     * This method updates the status of the specified repository in the internal map of repositories to disabled.
     *
     * @param a_name Name of the repository to disable
     * @return Whether the operation was successful
     */
    bool DisableRepository(const ntl::String& a_name);

    /**
     * @brief Lists all available repositories in the atlas package manager.
     */
    void ListRepositories();

    /**
     * @brief Fetches data from the configured repositories.
     *
     * This method triggers a fetch operation for each repository in the internal map of repositories.
     *
     * @return Whether the operation was successful
     */
    bool Fetch();

    /**
     * @brief Installs one or more packages using the atlas package manager.
     *
     * This method installs the specified package(s) using the PackageInstaller class.
     *
     * @param a_package_names Array of package names to install
     * @return Whether the operation was successful
     */
    bool Install(const ntl::Array<ntl::String>& a_package_names);

    /**
     * @brief Installs one package using the atlas package manager.
     *
     * This method installs the specified package using the PackageInstaller class.
     *
     * @param a_package_name Name of the package to install
     * @return Whether the operation was successful
     */
    bool Install(const ntl::String& a_package_name);

    /**
     * @brief Removes one or more packages from the atlas package manager.
     *
     * This method removes the specified package(s) using the PackageConfig class.
     *
     * @param a_package_name Name of the package to remove
     * @return Whether the operation was successful
     */
    bool Remove(const ntl::String& a_package_name);

    /**
     * @brief Updates all installed packages in the atlas package manager.
     *
     * This method updates each package using the PackageConfig class.
     *
     * @return Whether the operation was successful
     */
    bool Update();

    /**
     * @brief Upgrades one or more packages to the latest version available.
     *
     * This method upgrades the specified package(s) using the PackageConfig class.
     *
     * @param a_package_name Name of the package to upgrade
     * @return Whether the operation was successful
     */
    bool Upgrade(const ntl::String& a_package_name);

    /**
     * @brief Locks one or more packages in the atlas package manager.
     *
     * This method locks the specified package(s) using the PackageConfig class.
     *
     * @param a_package_name Name of the package to lock
     * @return Whether the operation was successful
     */
    bool LockPackage(const ntl::String& name);

    /**
     * @brief Unlocks one or more packages in the atlas package manager.
     *
     * This method unlocks the specified package(s) using the PackageConfig class.
     *
     * @param a_package_name Name of the package to unlock
     * @return Whether the operation was successful
     */
    bool UnlockPackage(const ntl::String& name);

    /**
     * @brief Cleans up any temporary files or data created during an installation process.
     */
    void Cleanup();

    /**
     * @brief Keeps one or more packages in the atlas package manager.
     *
     * This method keeps the specified package(s) using the PackageConfig class.
     *
     * @param a_package_name Name of the package to keep
     * @return Whether the operation was successful
     */
    bool KeepPackage(const ntl::String& name);

    /**
     * @brief Unkeeps one or more packages in the atlas package manager.
     *
     * This method unkeeps the specified package(s) using the PackageConfig class.
     *
     * @param a_package_name Name of the package to unkeep
     * @return Whether the operation was successful
     */
    bool UnkeepPackage(const ntl::String& name);

    /**
     * @brief Searches for packages matching a specific query in the atlas package manager.
     *
     * This method searches the internal map of repositories and returns a list of matching packages.
     *
     * @param a_query Search query
     * @return List of matching package names
     */
    std::vector<ntl::String> Search(const ntl::String& a_query);

    /**
     * @brief Displays information about one or more packages in the atlas package manager.
     *
     * This method displays information about the specified package(s) using the PackageConfig class.
     *
     * @param a_package_name Name of the package to display info for
     */
    void Info(const ntl::String& a_package_name);

    /**
     * @brief Checks whether one or more packages are installed in the atlas package manager.
     *
     * This method checks if the specified package(s) are installed using the PackageConfig class.
     *
     * @param a_package_name Name of the package to check
     * @return Whether the package is installed (default: false)
     */
    bool IsInstalled(const ntl::String& a_package_name) const;

    /**
     * @brief Configures and initializes the atlas package manager for first-time use.
     *
     * This method sets up the internal configuration, cache directory, and shortcut directory.
     *
     * @return Whether the operation was successful
     */
    bool AtlasSetup();

    /**
     * @brief Purges any remaining data or temporary files created during an installation process.
     *
     * This method cleans up any leftover data from previous installations.
     *
     * @return Whether the operation was successful
     */
    bool AtlasPurge();

  private:
    /**
     * @brief Loads all available repositories into memory for caching.
     */
    void loadRepositories();

    /**
     * @brief Saves the internal map of repositories to disk for persistence.
     */
    void saveRepositories();

    /**
     * @brief Loads package index data from disk or initializes it if not present.
     */
    void loadPackageIndex();

    /**
     * @brief Fetches data from a specific repository using the FetchData class.
     *
     * @param a_repo Repository to fetch from
     * @return Whether the operation was successful
     */
    bool fetchRepository(const Repository& a_repo) const;

    /**
     * @brief Removes one or more packages from the atlas package manager by updating the package index.
     *
     * @param a_config Package configuration to remove
     * @return Whether the operation was successful
     */
    bool removePackage(const PackageConfig& a_config);

    /**
     * @brief Records an installation event for one or more packages in the package index.
     *
     * @param a_config Package configuration to record
     */
    void recordInstallation(const PackageConfig& a_config);

    /**
     * @brief Records a removal event for one or more packages in the package index.
     *
     * @param a_config Package configuration to record
     */
    void recordRemoval(const PackageConfig& a_config);

    /**
     * @brief Upgrades one or more packages using the PackageInstaller class.
     *
     * @param a_config Package configuration to upgrade
     * @return Whether the operation was successful
     */
    bool upgrade(const PackageConfig& a_config);

    /**
     * @brief Returns the current date and time as a string.
     *
     * @return Current date and time in "YYYY-MM-DD HH:MM:SS" format
     */
    ntl::String getCurrentDateTime();

    /**
     * @brief Checks if the current operating system is macOS.
     *
     * @return Whether the current OS is macOS (default: false)
     */
    bool isMacOS();

    /**
     * @brief Cleans up any leftover packages in the atlas package manager.
     */
    void cleanupPackages();
  };
}

#endif // ATLAS_ATLAS_HPP
