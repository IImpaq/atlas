/**
* @file Config.cpp
* @author Marcus Gugacs
* @date 30.10.24
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#include "Config.hpp"

#include <fstream>
#include <iostream>

#include "Logger.hpp"

namespace atlas {
  fs::path Config::expandPath(const std::string& a_path) {
    if (a_path.empty() || a_path[0] != '~')
      return a_path;
    return fs::path(getenv("HOME")) / a_path.substr(2);
  }

  std::string Config::compressPath(const fs::path& a_path) {
    std::string home = getenv("HOME");
    std::string str_path = a_path.string();
    if (str_path.starts_with(home)) {
      return "~" + str_path.substr(home.length());
    }
    return str_path;
  }

  void Config::setDefaults() {
    fs::path home = fs::path(getenv("HOME"));
    m_core = {
      false
    };

    m_paths = {
      .install_dir = home / ".local/share/atlas",
      .cache_dir = home / ".cache/atlas",
      .shortcut_dir = home / ".local/share/applications"
    };

    m_network = {
      .timeout = 30,
      .retries = 3,
      .max_parallel_downloads = 4
    };
  }

  void Config::loadFromTable() {
    // Load core
    if (const auto& core = m_config["core"]) {
      if (const auto& verbose = core["verbose"].value<bool>())
        m_core.verbose = *verbose;
    }

    // Load paths
    if (const auto& paths = m_config["paths"]) {
      if (const auto& install = paths["install_dir"].value<std::string>())
        m_paths.install_dir = expandPath(*install);
      if (const auto& cache = paths["cache_dir"].value<std::string>())
        m_paths.cache_dir = expandPath(*cache);
      if (const auto& shortcuts = paths["shortcut_dir"].value<std::string>())
        m_paths.shortcut_dir = expandPath(*shortcuts);
    }

    // Load network settings
    if (const auto& network = m_config["network"]) {
      if (const auto& timeout = network["timeout"].value<int>())
        m_network.timeout = *timeout;
      if (const auto& retries = network["retries"].value<int>())
        m_network.retries = *retries;
      if (const auto& parallel = network["max_parallel_downloads"].value<int>())
        m_network.max_parallel_downloads = *parallel;
    }
  }

  void Config::updateTable() {
    // Update core
    if (!m_config.contains("core")) {
      m_config.insert("core", toml::table{});
    }

    auto& core = *m_config.get("core")->as_table();
    core.clear();
    core.insert("verbose", m_core.verbose);

    // Update paths
    if (!m_config.contains("paths")) {
      m_config.insert("paths", toml::table{});
    }
    auto& paths = *m_config.get("paths")->as_table();
    paths.clear();
    paths.insert("install_dir", compressPath(m_paths.install_dir));
    paths.insert("cache_dir", compressPath(m_paths.cache_dir));
    paths.insert("shortcut_dir", compressPath(m_paths.shortcut_dir));

    // Update network settings
    if (!m_config.contains("network")) {
      m_config.insert("network", toml::table{});
    }
    auto& network = *m_config.get("network")->as_table();
    network.clear();
    network.insert("timeout", m_network.timeout);
    network.insert("retries", m_network.retries);
    network.insert("max_parallel_downloads", m_network.max_parallel_downloads);
  }

  Config::Config(): m_config_path(fs::path(getenv("HOME")) / ".config/atlas/config.toml") {
    setDefaults();
    Load();
  }

  Config::Config(const fs::path& a_config_path): m_config_path(a_config_path) {
    setDefaults();
    Load();
  }

  void Config::Load() {
    if (fs::exists(m_config_path)) {
      try {
        m_config = toml::parse_file(m_config_path.string());
        loadFromTable();
      } catch (const toml::parse_error& err) {
        auto begin = err.source().begin;
        LOG_ERROR(ntl::String{"Error parsing config: "} + err.description().data() + "\n"
          + "Error at " + static_cast<int>(begin.line) + ":" + static_cast<int>(begin.column) + "\n");
        // Keep defaults on error
      }
    } else {
      // Create default config
      updateTable();
      Save();
    }
  }

  bool Config::Save() {
    try {
      fs::create_directories(m_config_path.parent_path());
      updateTable();
      std::ofstream file(m_config_path);
      file << m_config;
      return true;
    } catch (const std::exception& e) {
      LOG_ERROR(ntl::String{"Error saving config: "} + e.what() + "\n");
      return false;
    }
  }

  void Config::SetVerbose(bool a_verbose) {
    m_core.verbose = a_verbose;
    updateTable();
  }

  void Config::SetInstallDir(const fs::path& a_path) {
    m_paths.install_dir = a_path;
    updateTable();
  }

  void Config::SetCacheDir(const fs::path& a_path) {
    m_paths.cache_dir = a_path;
    updateTable();
  }

  void Config::SetShortcutDir(const fs::path& a_path) {
    m_paths.shortcut_dir = a_path;
    updateTable();
  }

  void Config::SetTimeout(int a_seconds) {
    m_network.timeout = a_seconds;
    updateTable();
  }

  void Config::SetRetries(int a_count) {
    m_network.retries = a_count;
    updateTable();
  }

  void Config::SetMaxParallelDownloads(int a_count) {
    m_network.max_parallel_downloads = a_count;
    updateTable();
  }
}
