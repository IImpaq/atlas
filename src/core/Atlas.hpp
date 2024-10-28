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
    fs::path installDir;
    fs::path cacheDir;
    fs::path shortcutDir;
    fs::path repoConfigPath;
    fs::path logDir;
    std::unordered_map<std::string, Repository> repositories;
    std::unordered_map<std::string, PackageConfig> packageIndex;

public:
  Atlas(const fs::path &install, const fs::path &cache, bool verbose);

  bool addRepository(const std::string &name, const std::string &url,
      const std::string &branch = "main");
  bool removeRepository(const std::string &name);
  bool enableRepository(const std::string &name);
  bool disableRepository(const std::string &name);
  void listRepositories();
  bool fetch();

  bool install(const std::string &packageName);
  bool update();
  bool remove(const std::string &packageName);

  std::vector<std::string> search(const std::string &query);
  void info(const std::string &packageName);

  bool isInstalled(const std::string &packageName) const;

private:
  void loadRepositories();
  void saveRepositories();
  void loadPackageIndex();

  bool fetchRepository(const Repository &repo) const;
  bool installPackage(const PackageConfig &config);
  void recordInstallation(const PackageConfig &config);
  void recordRemoval(const PackageConfig &config);

  std::string getCurrentDateTime();

  bool removePackage(const PackageConfig &config);

  bool isMacOS();

  fs::path getDefaultShortcutDir();

  void createShortcut(const std::string &repo);
  void createMacOSShortcut(const std::string &repo);
  void createLinuxShortcut(const std::string &repo);

  bool downloadRepository(const std::string &username, const std::string &repo) const;

  bool extractPackage(const std::string &repo) const;
};

#endif // ATLAS_ATLAS_HPP
