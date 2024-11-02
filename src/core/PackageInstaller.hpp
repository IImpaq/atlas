/**
* @file PackageInstaller.hpp
* @author Marcus Gugacs
* @date 28.10.24
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#ifndef ATLAS_PACKAGE_INSTALLER_HPP
#define ATLAS_PACKAGE_INSTALLER_HPP

#include <filesystem>
#include <regex>

#include <data/String.hpp>
#include <json/json.h>

#include "pods/PackageConfig.hpp"

namespace fs = std::filesystem;

namespace atlas {
  /**
   * @class PackageInstaller
   * @brief Provides functionality to install a package.
   *
   * This class is responsible for downloading, building, and installing a package.
   */
  class PackageInstaller {
  private:
    fs::path m_cache_dir;
    fs::path m_install_dir;
    fs::path m_log_dir;
    Json::Value m_config;
    ntl::String m_platform;

  public:
    /**
     * @brief Constructor.
     *
     * Initializes the PackageInstaller with the specified cache, install, and log directories,
     * as well as a package configuration.
     *
     * @param a_cache Cache directory
     * @param a_install Install directory
     * @param a_log Log directory
     * @param a_package_config Package configuration
     */
    PackageInstaller(const fs::path &a_cache, const fs::path &a_install,
                     const fs::path &a_log, const PackageConfig &a_package_config);

    /**
     * @brief Downloads the package.
     *
     * Performs any necessary downloads to obtain the package's dependencies.
     *
     * @return True if successful, false otherwise
     */
    bool Download();

    /**
     * @brief Prepares the package for installation.
     *
     * Resolves dependencies and prepares the package for building.
     *
     * @return True if successful, false otherwise
     */
    bool Prepare();

    /**
     * @brief Builds the package.
     *
     * Compiles and builds the package's components.
     *
     * @return True if successful, false otherwise
     */
    bool Build();

    /**
     * @brief Installs the package.
     *
     * Performs any necessary installation steps to make the package available on the system.
     *
     * @return True if successful, false otherwise
     */
    bool Install();

    /**
     * @brief Cleans up after package installation.
     *
     * Removes temporary files and performs any other cleanup tasks.
     *
     * @return True if successful, false otherwise
     */
    bool Cleanup();

    /**
     * @brief Uninstalls the package.
     *
     * Reverses the installation process to remove the package from the system.
     *
     * @return True if successful, false otherwise
     */
    bool Uninstall();

  private:
    /**
     * @brief Executes a sequence of shell commands.
     *
     * Runs each command in the specified array and returns true if all commands execute successfully,
     * or false otherwise.
     *
     * @param a_commands Array of commands to execute
     * @return True if successful, false otherwise
     */
    bool executeCommands(const Json::Value &a_commands);

    /**
     * @brief Replaces placeholders in a shell command with actual values.
     *
     * Takes a command string and replaces any placeholders (e.g. ${VARIABLE}) with the actual value of the variable.
     *
     * @param a_cmd Command string to modify
     * @return Modified command string with placeholders replaced
     */
    ntl::String replaceVariables(const ntl::String &a_cmd);

    /**
     * @brief Downloads a file from a URL.
     *
     * Retrieves a file from the specified URL and saves it to the target location.
     *
     * @param a_url URL of the file to download
     * @param a_target Target path where the file will be saved
     * @return True if successful, false otherwise
     */
    bool downloadFile(const ntl::String &a_url, const ntl::String &a_target);
  };
}

#endif // ATLAS_PACKAGE_INSTALLER_HPP
