//
// Created by Marcus Gugacs on 28.10.24.
//

#include "Atlas.hpp"

#include <set>

#include <toml++/toml.hpp>

#include "Logger.hpp"
#include "utils/JobSystem.hpp"
#include "utils/Misc.hpp"

namespace atlas {
  static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((ntl::String *) userp)->Append(static_cast<char *>(contents));
    return size * nmemb;
  }

  Atlas::Atlas(const fs::path &a_install, const fs::path &a_cache, bool verbose)
    : m_config(),
      m_repo_config_path(m_install_dir / "repositories.json"),
      m_log_dir(m_install_dir / "logs"), m_repositories(), m_package_index() {
    JobSystem::Instance().Initialize();
    Logger::Instance().Initialize();

    // Use config values
    m_install_dir = m_config.GetPaths().install_dir;
    m_cache_dir = m_config.GetPaths().cache_dir;
    m_shortcut_dir = m_config.GetPaths().shortcut_dir;

    fs::create_directories(m_install_dir);
    fs::create_directories(m_cache_dir);
    fs::create_directories(m_shortcut_dir);
    fs::create_directories(m_log_dir);
    loadRepositories();
    loadPackageIndex();
  }

  Atlas::~Atlas() {
    JobSystem::Instance().WaitForJobsToFinish();
    Logger::Instance().Shutdown();
    JobSystem::Instance().Shutdown();
  }

  bool Atlas::AddRepository(const ntl::String &a_name, const ntl::String &a_url, const ntl::String &a_branch) {
    if (m_repositories.Find(a_name) != m_repositories.end()) {
      LOG_ERROR("Repository already exists\n");
      return false;
    }

    Repository repo{a_name, a_url, a_branch, true};
    m_repositories[a_name] = repo;
    saveRepositories();
    return fetchRepository(repo);
  }

  bool Atlas::RemoveRepository(const ntl::String &a_name) {
    if (m_repositories.Find(a_name) == m_repositories.end()) {
      LOG_ERROR("Repository not found\n");
      return false;
    }
    m_repositories.Remove(a_name);
    fs::remove_all(m_cache_dir / a_name.GetCString());
    saveRepositories();
    loadPackageIndex();
    return true;
  }

  bool Atlas::EnableRepository(const ntl::String &a_name) {
    if (m_repositories.Find(a_name) == m_repositories.end()) {
      LOG_ERROR("Repository '" + a_name + "' not found...");
      return false;
    }
    m_repositories[a_name].enabled = true;
    saveRepositories();
    loadPackageIndex();
    LOG_MSG("Repository '" + a_name + "' enabled!");
    return true;
  }

  bool Atlas::DisableRepository(const ntl::String &a_name) {
    if (m_repositories.Find(a_name) == m_repositories.end()) {
      LOG_ERROR("Repository '" + a_name + "' not found...");
      return false;
    }
    m_repositories[a_name].enabled = false;
    saveRepositories();
    loadPackageIndex();
    LOG_MSG("Repository '" + a_name + "' disabled!");
    return true;
  }

  void Atlas::ListRepositories() {
    LOG_MSG("Local repositories:");
    for (const auto &[name, repo] : m_repositories) {
      LOG_MSG(name + " (" + (repo.enabled ? "enabled" : "disabled") + ")\n"
                + "  URL: " + repo.url + "\n" + "  Branch: " + repo.branch + "\n");
    }
  }

  bool Atlas::Fetch() {
    bool success = true;
    LoadingAnimation loadingAnimation("Fetching repositories");
    fs::path tempDir = m_cache_dir / "temp";
    fs::create_directories(tempDir);

    for (const auto &[name, repo]: m_repositories) {
      if (!repo.enabled)
        continue;

      LOG_MSG("Fetching " + name + "...\n");
      if (!fetchRepository(repo)) {
        LOG_ERROR("Failed to fetch repository: " + name + "\n");
        success = false;
        continue;
      }

      fs::path repoPath = m_cache_dir / name.GetCString();
      if (fs::exists(repoPath / "packages.json")) {
        try {
          Json::Value root;
          std::ifstream index_file(repoPath / "packages.json");
          index_file >> root;

          for (const auto &package: root["packages"]) {
            PackageConfig config{
              package["name"].asString().c_str(),
              package["version"].asString().c_str(),
              package["description"].asString().c_str(),
              package["build_command"].asString().c_str(),
              package["install_command"].asString().c_str(),
              package["uninstall_command"].asString().c_str(),
              name,
              ntl::Array<ntl::String>()
            };

            const Json::Value &deps = package["dependencies"];
            for (const auto &dep: deps) {
              config.dependencies.Insert(dep.asString().c_str());
            }
            m_package_index[config.name] = config;
          }
        } catch (const std::exception &e) {
          LOG_ERROR("Error parsing package index for " + name + ": " + e.what() + "\n");
          success = false;
        }
      }
    }

    fs::remove_all(tempDir);
    loadingAnimation.Stop();
    return success;
  }

  bool Atlas::Install(const ntl::String &a_package_name) {
    if (m_package_index.Find(a_package_name) == m_package_index.end()) {
      LOG_ERROR("Package not found\n");
      return false;
    }
    return installPackage(m_package_index[a_package_name]);
  }

  bool Atlas::Remove(const ntl::String &a_package_name) {
    if (m_package_index.Find(a_package_name) == m_package_index.end()) {
      LOG_ERROR("Package not found\n");
      return false;
    }
    return removePackage(m_package_index[a_package_name]);
  }

  bool Atlas::Update() {
    bool success = true;
    fs::path dbPath = m_install_dir / "installed.json";
    Json::Value root;

    if (fs::exists(dbPath)) {
      std::ifstream dbFile(dbPath);
      dbFile >> root;
    }

    bool installedSomething = false;

    for (const auto &[name, config]: m_package_index) {
      if (IsInstalled(name)) {
        ntl::String localVersion = root[name.GetCString()]["version"].asString().c_str();
        bool isLocked = root[name.GetCString()]["locked"].asBool();

        if (isLocked) {
          continue;
        }

        if (config.version != localVersion) {
          installedSomething = true;
          LOG_MSG("Updating " + name + " from version " + localVersion + " to " + config.version + "...\n");
          success &= installPackage(config);
        }
      }
    }

    if (!installedSomething) {
      LOG_MSG("No updates found\n");
    }

    return success;
  }

  bool Atlas::LockPackage(const ntl::String& name) {
    fs::path dbPath = m_install_dir / "installed.json";
    Json::Value root;

    if (!IsInstalled(name)) {
      LOG_ERROR("Package not installed\n");
      return false;
    }

    if (fs::exists(dbPath)) {
      std::ifstream dbFile(dbPath);
      dbFile >> root;
      root[name.GetCString()]["locked"] = true;
      std::ofstream outFile(dbPath);
      outFile << root;

      LOG_MSG("Locked package " + name + "!\n");
    }

    return true;
  }

  bool Atlas::UnlockPackage(const ntl::String& name) {
    fs::path dbPath = m_install_dir / "installed.json";
    Json::Value root;

    if (fs::exists(dbPath)) {
      std::ifstream dbFile(dbPath);
      dbFile >> root;
      root[name.GetCString()]["locked"] = false;
      std::ofstream outFile(dbPath);
      outFile << root;
      LOG_MSG("Unlocked package " + name + "!\n");
    }

    return true;
  }

  void Atlas::Cleanup() {
    LOG_MSG("Finding orphaned packages to remove...\n");
    cleanupPackages();
  }

  bool Atlas::KeepPackage(const ntl::String& name) {
    fs::path dbPath = m_install_dir / "installed.json";
    Json::Value root;

    if (!IsInstalled(name)) {
      LOG_ERROR("Package not installed\n");
      return false;
    }

    if (fs::exists(dbPath)) {
      std::ifstream dbFile(dbPath);
      dbFile >> root;
      root[name.GetCString()]["keep"] = true;
      std::ofstream outFile(dbPath);
      outFile << root;

      LOG_MSG("Keeping package " + name + "!\n");
    }
    return true;
  }

  bool Atlas::UnkeepPackage(const ntl::String& name) {
    fs::path dbPath = m_install_dir / "installed.json";
    Json::Value root;

    if (fs::exists(dbPath)) {
      std::ifstream dbFile(dbPath);
      dbFile >> root;
      root[name.GetCString()]["keep"] = false;
      std::ofstream outFile(dbPath);
      outFile << root;

      LOG_MSG("Not keeping package " + name + "!\n");
    }
    return true;
  }

  std::vector<ntl::String> Atlas::Search(const ntl::String &a_query) {
    std::vector<ntl::String> results;
    std::string str;
    for (const auto &[name, config]: m_package_index) {
      if (name.Find(a_query) != -1 || config.description.Find(a_query) != -1) {
        results.push_back(name);
      }
    }
    return results;
  }

  void Atlas::Info(const ntl::String &a_package_name) {
    if (m_package_index.Find(a_package_name) == m_package_index.end()) {
      LOG_ERROR("Package not found\n");
      return;
    }

    const auto &config = m_package_index[a_package_name];
    LOG_MSG("Name: " + config.name + "\n"
        + "Version: " + config.version + "\n"
        + "Description: " + config.description + "\n"
        + "Status: " + (IsInstalled(a_package_name) ? GREEN : RED)
        + (IsInstalled(a_package_name) ? "Installed" : "Not installed")
        + "\n");
  }

  bool Atlas::IsInstalled(const ntl::String &a_package_name) const {
    fs::path dbPath = m_install_dir / "installed.json";
    if (!fs::exists(dbPath)) {
      return false;
    }

    Json::Value root;
    std::ifstream dbFile(dbPath);
    dbFile >> root;

    return root.isMember(a_package_name.GetCString());
  }

  void Atlas::loadRepositories() {
    if (!fs::exists(m_repo_config_path)) {
      return;
    }

    Json::Value root;
    std::ifstream config_file(m_repo_config_path);
    config_file >> root;

    for (const auto &repo: root["repositories"]) {
      Repository r{
        repo["name"].asString().c_str(), repo["url"].asString().c_str(),
        repo["branch"].asString().c_str(), repo["enabled"].asBool()
      };
      m_repositories[r.name] = r;
    }
  }

  void Atlas::saveRepositories() {
    Json::Value root;
    Json::Value repoArray(Json::arrayValue);

    for (const auto &[name, repo]: m_repositories) {
      Json::Value repoObj;
      repoObj["name"] = repo.name.GetCString();
      repoObj["url"] = repo.url.GetCString();
      repoObj["branch"] = repo.branch.GetCString();
      repoObj["enabled"] = repo.enabled;
      repoArray.append(repoObj);
    }
    root["repositories"] = repoArray;

    std::ofstream config_file(m_repo_config_path);
    config_file << root;
  }

  void Atlas::loadPackageIndex() {
    m_package_index.Clear();
    for (const auto &[name, repo]: m_repositories) {
      if (!repo.enabled)
        continue;

      fs::path repoPath = m_cache_dir / name.GetCString();
      for (const auto &entry: fs::recursive_directory_iterator(repoPath)) {
        if (entry.path().filename() == "package.json") {
          Json::Value root;
          std::ifstream config_file(entry.path());
          config_file >> root;

          PackageConfig config{
            root["name"].asString().c_str(),
            root["version"].asString().c_str(),
            root["description"].asString().c_str(),
            root["build_command"].asString().c_str(),
            root["install_command"].asString().c_str(),
            root["uninstall_command"].asString().c_str(),
            name,
            ntl::Array<ntl::String>()
          };
          m_package_index[config.name] = config;
        }
      }
    }
  }

  bool Atlas::fetchRepository(const Repository &a_repo) const {
    fs::path repoPath = m_cache_dir / a_repo.name.GetCString();
    fs::path zipPath = m_cache_dir / (a_repo.name + ".zip").GetCString();

    CURL *curl = curl_easy_init();
    if (!curl) {
      LOG_ERROR("Failed to initialize CURL\n");
      return false;
    }

    ntl::String url =
        "https://api.github.com/repos/" + a_repo.url + "/zipball/" + a_repo.branch;
    FILE *fp = fopen(zipPath.string().c_str(), "wb");
    if (!fp) {
      curl_easy_cleanup(curl);
      LOG_ERROR("Failed to create zip file\n");
      return false;
    }

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Accept: application/vnd.github+json");
    headers = curl_slist_append(headers, "User-Agent: Atlas-Package-Manager");

    curl_easy_setopt(curl, CURLOPT_URL, url.GetCString());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, nullptr);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
      LOG_ERROR(ntl::String{"Download failed: "} + curl_easy_strerror(res) + "\n");
      return false;
    }

    if (fs::exists(repoPath)) {
      fs::remove_all(repoPath);
    }
    fs::create_directories(repoPath);

    ntl::String cmd = ntl::String{"unzip -o "} + zipPath.string().c_str() + " -d " + repoPath.string().c_str();
    int extract_result = ProcessCommand(cmd, m_log_dir.string().c_str() + ntl::String{"/latest.log"}, m_config.GetCore().verbose);
    fs::remove(zipPath);

    if (extract_result != 0) {
      LOG_ERROR("Failed to extract repository\n");
      return false;
    }

    fs::path nested_dir;
    for (const auto &entry: fs::directory_iterator(repoPath)) {
      if (fs::is_directory(entry)) {
        nested_dir = entry.path();
        break;
      }
    }

    if (!nested_dir.empty()) {
      for (const auto &entry: fs::directory_iterator(nested_dir)) {
        fs::rename(entry.path(), repoPath / entry.path().filename());
      }
      fs::remove_all(nested_dir);
    }

    return true;
  }

  bool Atlas::installPackage(const PackageConfig &a_config) {
    // First install all dependencies
    for (const auto& dep : a_config.dependencies) {
      if (m_package_index.Find(dep) == m_package_index.end()) {
        LOG_ERROR("Unknown dependency " + dep + "\n");
        return false;
      }

      PackageConfig dep_config = m_package_index[dep];
      if (!installPackage(dep_config)) {
        LOG_ERROR("Failed to install dependency: " + dep + "\n");
        return false;
      }
    }

    // Then proceed with the main package installation
    PackageInstaller installer(m_cache_dir, m_install_dir, m_log_dir, a_config);
    LoadingAnimation loading("Installing " + a_config.name);

    bool success = installer.Download() && installer.Prepare() &&
                  installer.Build() && installer.Install() &&
                  installer.Cleanup();

    loading.Stop();

    if (!success) {
      LOG_ERROR("Installation failed for " + a_config.name + "\n");
      return false;
    }

    recordInstallation(a_config);
    return true;
  }

  bool Atlas::removePackage(const PackageConfig &a_config) {
    PackageInstaller installer(m_cache_dir, m_install_dir, m_log_dir, a_config);

    LoadingAnimation loading("Removing " + a_config.name);

    bool success = installer.Uninstall();

    loading.Stop();

    if (!success) {
      LOG_ERROR("Removal failed for " + a_config.name + "\n");
      return false;
    }

    recordRemoval(a_config);

    return true;
  }

  void Atlas::recordInstallation(const PackageConfig &a_config) {
    Json::Value root;
    fs::path db_path = m_install_dir / "installed.json";

    if (fs::exists(db_path)) {
      std::ifstream db_file(db_path);
      db_file >> root;
    }

    Json::Value package;
    package["version"] = a_config.version.GetCString();
    package["install_date"] = getCurrentDateTime().GetCString();
    package["repository"] = a_config.repository.GetCString();
    package["locked"] = false;

    root[a_config.name.GetCString()] = package;

    std::ofstream db_file(db_path);
    db_file << root;
  }

  void Atlas::recordRemoval(const PackageConfig &a_config) {
    Json::Value root;
    fs::path db_path = m_install_dir / "installed.json";

    if (fs::exists(db_path)) {
      std::ifstream db_file(db_path);
      db_file >> root;
    }

    root.removeMember(a_config.name.GetCString());

    std::ofstream db_file(db_path);
    db_file << root;
  }

  ntl::String Atlas::getCurrentDateTime() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    ntl::String datetime = std::ctime(&time);
    // TODO: Implement -> datetime.pop_back();
    return datetime;
  }


  bool Atlas::isMacOS() {
  #ifdef __APPLE__
    return true;
  #else
          return false;
  #endif
  }

  bool Atlas::downloadRepository(const ntl::String &a_username, const ntl::String &a_repo) const {
    CURL *curl = curl_easy_init();
    ntl::String url = "https://api.github.com/repos/" + a_username + "/" + a_repo +
                      "/zipball/master";
    ntl::String zipPath = (m_cache_dir / (a_repo + ".zip").GetCString()).string().c_str();

    if (!curl)
      return false;

    FILE *fp = fopen(zipPath.GetCString(), "wb");
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Accept: application/vnd.github+json");
    headers = curl_slist_append(headers, "User-Agent: Atlas-Package-Manager");

    curl_easy_setopt(curl, CURLOPT_URL, url.GetCString());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, nullptr);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return res == CURLE_OK;
  }

  bool Atlas::extractPackage(const ntl::String &a_repo) const {
    ntl::String zipPath = (m_cache_dir / (a_repo + ".zip").GetCString()).c_str();
    ntl::String extractPath = (m_install_dir / a_repo.GetCString()).c_str();
    ntl::String cmd = "unzip -o " + zipPath + " -d " + extractPath;
    return system(cmd.GetCString()) == 0;
  }

  void Atlas::cleanupPackages() {
    fs::path dbPath = m_install_dir / "installed.json";
    if (!fs::exists(dbPath)) {
      return;
    }

    Json::Value root;
    std::ifstream dbFile(dbPath);
    dbFile >> root;

    // Create a set of all dependencies
    std::set<ntl::String> allDependencies;
    for (const auto& packageName : root.getMemberNames()) {
      const auto& deps = root[packageName]["dependencies"];
      for (const auto& dep : deps) {
        allDependencies.insert(dep.asString().c_str());
      }
    }

    // Check each installed package
    for (const auto& packageName : root.getMemberNames()) {
      // Skip if package is a dependency of another package
      if (allDependencies.contains(packageName.c_str())) {
        continue;
      }

      // Skip if package is marked to keep
      bool keepPackage = root[packageName]["keep"].asBool();
      if (keepPackage) {
        LOG_MSG(ntl::String{"Package '"} + packageName.c_str() + "' is marked to keep and will not be removed.\n");
        continue;
      }

      LOG_MSG(ntl::String{"Package '"} + packageName.c_str() + "' is not required by any other package.\n");
      LOG_MSG("Do you want to remove it? (y/n): ");

      char response;
      std::cin >> response;

      if (std::tolower(response) == 'y') {
        if (remove(packageName.c_str())) {
          LOG_MSG(ntl::String{"Successfully removed "} + packageName.c_str() + "\n");
        } else {
          LOG_MSG(ntl::String{"Failed to remove "} + packageName.c_str() + "\n");
        }
      }
    }
  }
}
