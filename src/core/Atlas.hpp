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
#include <unordered_map>
#include <vector>

#include "pods/PackageConfig.hpp"
#include "pods/Repository.hpp"
#include "utils/LoadingAnimation.hpp"
#include "core/PackageInstaller.hpp"

namespace fs = std::filesystem;

class Atlas {
private:
  bool verbose;
  fs::path m_install_dir;
  fs::path m_cache_dir;
  fs::path m_shortcut_dir;
  fs::path m_repo_config_path;
  fs::path m_log_dir;
  std::unordered_map<std::string, Repository> m_repositories;
  std::unordered_map<std::string, PackageConfig> m_package_index;

public:
  Atlas(const fs::path &a_install, const fs::path &a_cache, bool a_verbose);

  bool AddRepository(const std::string &a_name, const std::string &a_url,
                     const std::string &a_branch = "main");

  bool RemoveRepository(const std::string &a_name);

  bool EnableRepository(const std::string &a_name);

  bool DisableRepository(const std::string &a_name);

  void ListRepositories();

  bool Fetch();

  bool Install(const std::string &a_package_name);

  bool Update();

  bool Remove(const std::string &a_package_name);

  std::vector<std::string> Search(const std::string &a_query);

  void Info(const std::string &a_package_name);

  bool IsInstalled(const std::string &a_package_name) const;

private:
  void loadRepositories();

  void saveRepositories();

  void loadPackageIndex();

  bool fetchRepository(const Repository &a_repo) const;

  bool installPackage(const PackageConfig &a_config);

  void recordInstallation(const PackageConfig &a_config);

  void recordRemoval(const PackageConfig &a_config);

  std::string getCurrentDateTime();

  bool removePackage(const PackageConfig &a_config);

  bool isMacOS();

  fs::path getDefaultShortcutDir();

  void createShortcut(const std::string &a_repo);

  void createMacOSShortcut(const std::string &a_repo);

  void createLinuxShortcut(const std::string &a_repo);

  bool downloadRepository(const std::string &a_username, const std::string &a_repo) const;

  bool extractPackage(const std::string &a_repo) const;
};

#endif // ATLAS_ATLAS_HPP
