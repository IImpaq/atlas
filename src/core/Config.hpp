/**
* @file Config.hpp
* @author Marcus Gugacs
* @date 30.10.24
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#ifndef ATLAS_CONFIG_HPP
#define ATLAS_CONFIG_HPP

#include <toml++/toml.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace atlas {
  class Config {
  public:
    struct Core {
      bool verbose;
    };

    struct Paths {
      fs::path install_dir;
      fs::path cache_dir;
      fs::path shortcut_dir;
    };

    struct Network {
      int timeout;
      int retries;
      int max_parallel_downloads;
    };

  private:
    fs::path m_config_path;
    toml::table m_config;
    Core m_core;
    Paths m_paths;
    Network m_network;

  public:
    /**
     * @brief Default constructor. Loads configuration from default location.
     */
    Config();

    /**
     * @brief Constructor with explicit configuration path. Loads configuration from specified location.
     *
     * @param a_config_path The absolute path to the TOML file containing the configuration.
     */
    explicit Config(const fs::path &a_config_path);

    /**
     * @brief Loads configuration from the default or explicitly specified location.
     *
     * If no configuration exists, sets default values and returns true. Otherwise, updates existing configuration
     * with new values and returns false (if changes were made).
     */
    void Load();

    /**
     * @brief Saves current configuration to a TOML file.
     *
     * Returns true if successful, false otherwise.
     */
    bool Save();

    // Getters
    /**
     * @brief Returns the core configuration struct.
     *
     * @return The core configuration struct.
     */
    const Core &GetCore() const { return m_core; }

    /**
     * @brief Returns the paths configuration struct.
     *
     * @return The paths configuration struct.
     */
    const Paths &GetPaths() const { return m_paths; }

    /**
     * @brief Returns the network configuration struct.
     *
     * @return The network configuration struct.
     */
    const Network &GetNetwork() const { return m_network; }

    // Core setters
    /**
     * @brief Sets the verbose flag to the specified value.
     *
     * @param a_verbose True to enable verbosity, false otherwise.
     */
    void SetVerbose(bool a_verbose);

    // Path setters
    /**
     * @brief Sets the installation directory to the specified path.
     *
     * @param a_path The absolute path to the new installation directory.
     */
    void SetInstallDir(const fs::path &a_path);

    /**
     * @brief Sets the cache directory to the specified path.
     *
     * @param a_path The absolute path to the new cache directory.
     */
    void SetCacheDir(const fs::path &a_path);

    /**
     * @brief Sets the shortcut directory to the specified path.
     *
     * @param a_path The absolute path to the new shortcut directory.
     */
    void SetShortcutDir(const fs::path &a_path);

    // Network setters
    /**
     * @brief Sets the timeout value in seconds.
     *
     * @param a_seconds The new timeout value (in seconds).
     */
    void SetTimeout(int a_seconds);

    /**
     * @brief Sets the number of retries.
     *
     * @param a_count The new number of retries.
     */
    void SetRetries(int a_count);

    /**
     * @brief Sets the maximum number of parallel downloads.
     *
     * @param a_count The new maximum number of parallel downloads.
     */
    void SetMaxParallelDownloads(int a_count);

  private:
    /**
     * @brief Expands an absolute path by replacing any environment variables with their actual values.
     *
     * @param a_path The absolute path to expand.
     *
     * @return The expanded absolute path.
     */
    static fs::path expandPath(const std::string &a_path);

    /**
     * @brief Compresses an absolute path into a compact string representation.
     *
     * @param a_path The absolute path to compress.
     *
     * @return The compressed path as a string.
     */
    static std::string compressPath(const fs::path &a_path);

    /**
     * @brief Sets default values for all configuration members.
     */
    void setDefaults();

    /**
     * @brief Loads configuration from the specified TOML table.
     *
     * This method assumes that the table contains all required configuration keys and sets their values in
     * the corresponding struct fields.
     */
    void loadFromTable();

    /**
     * @brief Updates the internal TOML table with current configuration values.
     */
    void updateTable();
  };
}

#endif // ATLAS_CONFIG_HPP
