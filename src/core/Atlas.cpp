/**
* @file Atlas.cpp
* @author Marcus Gugacs
* @date 28.10.24
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#include "Atlas.hpp"

#include <set>

#include <toml++/toml.hpp>

#include "Logger.hpp"
#include "utils/JobSystem.hpp"
#include "utils/Misc.hpp"

namespace atlas {
  static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((ntl::String *) userp)->Append(static_cast<char *>(contents));
    return size * nmemb;
  }

  Atlas::Atlas(const fs::path& a_install, const fs::path& a_cache, bool verbose)
    : m_config(), m_install_dir(m_config.GetPaths().install_dir), m_cache_dir(m_config.GetPaths().cache_dir),
      m_shortcut_dir(m_config.GetPaths().shortcut_dir), m_repo_config_path(m_install_dir / "repositories.json"),
      m_log_dir(m_install_dir / "logs"), m_repositories(), m_package_index() {
    JobSystem::Instance().Initialize(m_config.GetNetwork().max_parallel_downloads);
    Logger::Instance().Initialize();

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

  bool Atlas::AddRepository(const ntl::String& a_name, const ntl::String& a_url, const ntl::String& a_branch) {
    if (m_repositories.Find(a_name) != m_repositories.end()) {
      LOG_ERROR("Repository already exists");
      return false;
    }

    Repository repo{a_name, a_url, a_branch, true};
    m_repositories[a_name] = repo;
    saveRepositories();
    return fetchRepository(repo);
  }

  bool Atlas::RemoveRepository(const ntl::String& a_name) {
    if (m_repositories.Find(a_name) == m_repositories.end()) {
      LOG_ERROR("Repository not found");
      return false;
    }
    m_repositories.Remove(a_name);
    fs::remove_all(m_cache_dir / a_name.GetCString());
    saveRepositories();
    loadPackageIndex();
    return true;
  }

  bool Atlas::EnableRepository(const ntl::String& a_name) {
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

  bool Atlas::DisableRepository(const ntl::String& a_name) {
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
    for (const auto& [name, repo] : m_repositories) {
      LOG_MSG(name + " (" + (repo.enabled ? "enabled" : "disabled") + ")\n"
        + "  URL: " + repo.url + "\n" + "  Branch: " + repo.branch);
    }
  }

  bool Atlas::Fetch() {
    fs::path tempDir = m_cache_dir / "temp";
    fs::create_directories(tempDir);

    // Schedule repository fetching jobs
    m_repositories_lock.StartRead();
    for (const auto& [name, repo] : m_repositories) {
      if (!repo.enabled) {
        continue;
      }

      JobSystem::Instance().AddJob([&, name, repo]() {
        m_animator.UpdateStatus(name, "Fetching");

        if (!fetchRepository(repo)) {
          LOG_ERROR("Failed to fetch repository: " + name);
          m_fetch_data_lock.StartWrite();
          m_fetch_data.failed_fetchs.Insert(name);
          m_fetch_data_lock.EndWrite();
          m_animator.RemovePackage(name);
          return;
        }

        m_animator.UpdateStatus(name, "Parsing");
        fs::path repoPath = m_cache_dir / name.GetCString();

        if (fs::exists(repoPath / "packages.json")) {
          try {
            Json::Value root; {
              std::ifstream index_file(repoPath / "packages.json");
              index_file >> root;
            }

            std::vector<PackageConfig> configs;
            for (const auto& package : root["packages"]) {
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

              const Json::Value& deps = package["dependencies"];
              for (const auto& dep : deps) {
                config.dependencies.Insert(dep.asString().c_str());
              }
              configs.push_back(config);
            }

            // Add all configs at once to minimize lock time
            m_package_index_lock.StartWrite();
            for (const auto& config : configs) {
              m_package_index[config.name] = config;
            }
            m_package_index_lock.EndWrite();
          } catch (const std::exception& e) {
            LOG_ERROR("Error parsing package index for " + name + ": " + e.what());
            m_fetch_data_lock.StartWrite();
            m_fetch_data.failed_fetchs.Insert(name);
            m_fetch_data_lock.EndWrite();
          }
        }

        m_animator.RemovePackage(name);
      });
    }
    m_repositories_lock.EndRead();

    JobSystem::Instance().WaitForJobsToFinish();

    fs::remove_all(tempDir);

    m_fetch_data_lock.StartRead();
    bool result = m_fetch_data.failed_fetchs.IsEmpty();
    m_fetch_data_lock.EndRead();

    return result;
  }


  bool Atlas::Install(const ntl::Array<ntl::String>& a_package_names) {
    // Validate all packages exist first
    m_installer_data_lock.StartWrite();
    for (const auto& name : a_package_names) {
      if (m_package_index.Find(name) == m_package_index.end()) {
        LOG_ERROR("Package not found: " + name);
        m_installer_data_lock.EndWrite();
        return false;
      }
      m_installer_data.configs.Insert(m_package_index[name]);
    }
    m_installer_data_lock.EndWrite();

    // Helper function to schedule package and its dependencies
    std::function<void(const PackageConfig&)> schedulePackage = [&](const PackageConfig& config) {
      m_installer_data_lock.StartRead();
      if (m_installer_data.scheduled[config.name] || !m_installer_data.failed_installs.IsEmpty()) {
        m_installer_data_lock.EndRead();
        m_installer_data_lock.StartWrite();
        m_installer_data.skipped_installs.Insert(config.name);
        m_installer_data_lock.EndWrite();
        return;
      }
      m_installer_data_lock.EndRead();

      // First schedule all dependencies
      m_package_index_lock.StartRead();
      for (const auto& dep : config.dependencies) {
        if (m_package_index.Find(dep) == m_package_index.end()) {
          m_package_index_lock.EndRead();
          m_installer_data_lock.StartWrite();
          m_installer_data.failed_installs.Insert(dep);
          m_installer_data_lock.EndWrite();
          LOG_ERROR("Unknown dependency " + dep);
          return;
        }
        schedulePackage(m_package_index[dep]);
      }
      m_package_index_lock.EndRead();

      m_installer_data_lock.StartWrite();
      if (!m_installer_data.failed_installs.IsEmpty()) {
        m_installer_data.skipped_installs.Insert(config.name);
        m_installer_data_lock.EndWrite();
        return;
      }

      m_installer_data.scheduled[config.name] = true;
      m_installer_data_lock.EndWrite();

      JobSystem::Instance().AddJob([&]() {
        PackageInstaller installer(m_cache_dir, m_install_dir, m_log_dir, config);

        m_animator.UpdateStatus(config.name, "Downloading");
        bool success = installer.Download();

        if (success) {
          m_animator.UpdateStatus(config.name, "Preparing");
          success = installer.Prepare();
        }

        if (success) {
          m_animator.UpdateStatus(config.name, "Building");
          success = installer.Build();
        }

        if (success) {
          m_animator.UpdateStatus(config.name, "Installing");
          success = installer.Install();
        }

        if (success) {
          m_animator.UpdateStatus(config.name, "Cleaning");
          success = installer.Cleanup();
        }

        m_animator.RemovePackage(config.name);

        if (!success) {
          LOG_ERROR("Installation failed for " + config.name);
          m_installer_data_lock.StartWrite();
          m_installer_data.failed_installs.Insert(config.name);
          m_installer_data_lock.EndWrite();
          return;
        }

        m_installer_data_lock.StartWrite();
        recordInstallation(config);
        m_installer_data_lock.EndWrite();
      });
    };

    // Schedule all packages and their dependencies
    for (const auto& config : m_installer_data.configs) {
      schedulePackage(config);
    }

    bool result = m_installer_data.failed_installs.IsEmpty();

    return result;
  }

  bool Atlas::Install(const ntl::String& a_package_name) {
    if (m_package_index.Find(a_package_name) == m_package_index.end()) {
      LOG_ERROR("Package not found");
      return false;
    }
    return Install({a_package_name});
  }

  bool Atlas::Remove(const ntl::String& a_package_name) {
    if (m_package_index.Find(a_package_name) == m_package_index.end()) {
      LOG_ERROR("Package not found");
      return false;
    }
    return removePackage(m_package_index[a_package_name]);
  }

  bool Atlas::Update() {
    fs::path dbPath = m_install_dir / "installed.json";
    Json::Value root;

    if (fs::exists(dbPath)) {
      std::ifstream dbFile(dbPath);
      dbFile >> root;
    }

    m_installer_data_lock.StartWrite();
    for (const auto& [name, config] : m_package_index) {
      m_installer_data.configs.Insert(config);
    }
    m_installer_data_lock.EndWrite();

    // Helper function to schedule package update
    std::function<void(const PackageConfig&)> scheduleUpdate = [&](const PackageConfig& config) {
      m_installer_data_lock.StartRead();
      if (m_installer_data.scheduled[config.name] || !m_installer_data.failed_installs.IsEmpty()) {
        m_installer_data_lock.EndRead();
        m_installer_data_lock.StartWrite();
        m_installer_data.skipped_installs.Insert(config.name);
        m_installer_data_lock.EndWrite();
        return;
      }
      m_installer_data_lock.EndRead();

      m_installer_data_lock.StartWrite();
      m_installer_data.scheduled[config.name] = true;
      m_installer_data_lock.EndWrite();

      JobSystem::Instance().AddJob([&, root]() {
        if (IsInstalled(config.name)) {
          ntl::String localVersion = root[config.name.GetCString()]["version"].asString().c_str();
          bool isLocked = root[config.name.GetCString()]["locked"].asBool();

          if (!isLocked && config.version != localVersion) {
            LOG_MSG("Updating " + config.name + " from version " + localVersion + " to " + config.version + "...");

            PackageInstaller installer(m_cache_dir, m_install_dir, m_log_dir, config);

            m_animator.UpdateStatus(config.name, "Downloading");
            bool packageSuccess = installer.Download();

            if (packageSuccess) {
              m_animator.UpdateStatus(config.name, "Preparing");
              packageSuccess = installer.Prepare();
            }

            if (packageSuccess) {
              m_animator.UpdateStatus(config.name, "Building");
              packageSuccess = installer.Build();
            }

            if (packageSuccess) {
              m_animator.UpdateStatus(config.name, "Installing");
              packageSuccess = installer.Install();
            }

            if (packageSuccess) {
              m_animator.UpdateStatus(config.name, "Cleaning");
              packageSuccess = installer.Cleanup();
            }

            m_animator.RemovePackage(config.name);

            if (!packageSuccess) {
              LOG_ERROR("Update failed for " + config.name);
              m_installer_data_lock.StartWrite();
              m_installer_data.failed_installs.Insert(config.name);
              m_installer_data_lock.EndWrite();
              return;
            }

            m_installer_data_lock.StartWrite();
            m_installer_data.successful_installs.Insert(config.name);
            recordInstallation(config);
            m_installer_data_lock.EndWrite();
          }
        }
      });
    };

    // Schedule updates for all packages
    for (const auto& config : m_installer_data.configs) {
      scheduleUpdate(config);
    }

    // Wait for all jobs to complete
    JobSystem::Instance().WaitForJobsToFinish();

    m_installer_data_lock.StartRead();
    if (m_installer_data.successful_installs.IsEmpty()) {
      LOG_WARN("No updates found");
    }

    bool success = m_installer_data.failed_installs.IsEmpty();
    m_installer_data_lock.EndRead();

    return success;
  }

  bool Atlas::Upgrade(const ntl::String& a_package_name) {
    if (m_package_index.Find(a_package_name) == m_package_index.end()) {
      LOG_ERROR("Package not found");
      return false;
    }

    return upgrade(m_package_index[a_package_name]);
  }

  bool Atlas::LockPackage(const ntl::String& name) {
    fs::path dbPath = m_install_dir / "installed.json";
    Json::Value root;

    if (!IsInstalled(name)) {
      LOG_ERROR("Package not installed");
      return false;
    }

    if (fs::exists(dbPath)) {
      std::ifstream dbFile(dbPath);
      dbFile >> root;
      root[name.GetCString()]["locked"] = true;
      std::ofstream outFile(dbPath);
      outFile << root;

      LOG_MSG("Locked package " + name + "!");
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
      LOG_MSG("Unlocked package " + name + "!");
    }

    return true;
  }

  void Atlas::Cleanup() {
    LOG_MSG("Finding orphaned packages to remove...");
    cleanupPackages();
  }

  bool Atlas::KeepPackage(const ntl::String& name) {
    fs::path dbPath = m_install_dir / "installed.json";
    Json::Value root;

    if (!IsInstalled(name)) {
      LOG_ERROR("Package not installed");
      return false;
    }

    if (fs::exists(dbPath)) {
      std::ifstream dbFile(dbPath);
      dbFile >> root;
      root[name.GetCString()]["keep"] = true;
      std::ofstream outFile(dbPath);
      outFile << root;

      LOG_MSG("Keeping package " + name + "!");
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

      LOG_MSG("Not keeping package " + name + "!");
    }
    return true;
  }

  std::vector<ntl::String> Atlas::Search(const ntl::String& a_query) {
    std::vector<ntl::String> results;
    std::string str;
    for (const auto& [name, config] : m_package_index) {
      if (name.Find(a_query) != -1 || config.description.Find(a_query) != -1) {
        results.push_back(name);
      }
    }
    return results;
  }

  void Atlas::Info(const ntl::String& a_package_name) {
    if (m_package_index.Find(a_package_name) == m_package_index.end()) {
      LOG_ERROR("Package not found");
      return;
    }

    const auto& config = m_package_index[a_package_name];
    LOG_MSG("Name: " + config.name + "\n"
      + "Version: " + config.version + "\n"
      + "Description: " + config.description + "\n"
      + "Status: " + (IsInstalled(a_package_name) ? GREEN : RED)
      + (IsInstalled(a_package_name) ? "Installed" : "Not installed")
    );
  }

  bool Atlas::IsInstalled(const ntl::String& a_package_name) const {
    fs::path dbPath = m_install_dir / "installed.json";
    if (!fs::exists(dbPath)) {
      return false;
    }

    Json::Value root;
    std::ifstream dbFile(dbPath);
    dbFile >> root;

    return root.isMember(a_package_name.GetCString());
  }

  bool Atlas::AtlasSetup() {
    fs::path currentExePath = fs::current_path() / "atlas";
    fs::path homeDir = getenv("HOME");
    fs::path binDir = homeDir / ".local/bin";
    fs::path installPath = binDir / "atlas";

    try {
      // Create user bin directory if it doesn't exist
      fs::create_directories(binDir);

      // Copy executable
      fs::copy(currentExePath, installPath, fs::copy_options::overwrite_existing);

      // Set executable permissions
      fs::permissions(installPath, fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec);

      // Add to PATH if not already present
      std::string pathAdd = "\nexport PATH=\"$HOME/.local/bin:$PATH\"\n";

      // Check and update .bashrc
      std::string bashrcPath = (homeDir / ".bashrc").string();
      if (fs::exists(bashrcPath)) {
        std::ifstream bashrcRead(bashrcPath);
        std::string content((std::istreambuf_iterator<char>(bashrcRead)),
                            std::istreambuf_iterator<char>());

        if (content.find(".local/bin:$PATH") == std::string::npos) {
          std::ofstream bashrc(bashrcPath, std::ios_base::app);
          bashrc << pathAdd;
          LOG_MSG("Added to PATH in .bashrc");
        }
      }

      // Check and update .zshrc
      std::string zshrcPath = (homeDir / ".zshrc").string();
      if (fs::exists(zshrcPath)) {
        std::ifstream zshrcRead(zshrcPath);
        std::string content((std::istreambuf_iterator<char>(zshrcRead)),
                            std::istreambuf_iterator<char>());

        if (content.find(".local/bin:$PATH") == std::string::npos) {
          std::ofstream zshrc(zshrcPath, std::ios_base::app);
          zshrc << pathAdd;
          LOG_MSG("Added to PATH in .zshrc");
        }
      }

      LOG_INFO("Installation complete. Please restart your terminal or run 'source ~/.bashrc' (or ~/.zshrc)");
      return true;
    } catch (const std::exception& e) {
      LOG_ERROR("Installation failed: " + ntl::String(e.what()));
      return false;
    }
  }

  bool Atlas::AtlasPurge() {
    fs::path homeDir = getenv("HOME");
    fs::path installPath = homeDir / ".local/bin/atlas";

    try {
      if (fs::exists(installPath)) {
        fs::remove(installPath);
        LOG_INFO("Uninstallation complete. You may want to remove the PATH addition from ~/.bashrc and ~/.zshrc");
        return true;
      }
      return false;
    } catch (const std::exception& e) {
      LOG_ERROR("Uninstallation failed: " + ntl::String(e.what()));
      return false;
    }
  }


  void Atlas::loadRepositories() {
    if (!fs::exists(m_repo_config_path)) {
      return;
    }

    Json::Value root;
    std::ifstream config_file(m_repo_config_path);
    config_file >> root;

    for (const auto& repo : root["repositories"]) {
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

    for (const auto& [name, repo] : m_repositories) {
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
    for (const auto& [name, repo] : m_repositories) {
      if (!repo.enabled)
        continue;

      fs::path repoPath = m_cache_dir / name.GetCString();
      for (const auto& entry : fs::recursive_directory_iterator(repoPath)) {
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

  bool Atlas::fetchRepository(const Repository& a_repo) const {
    fs::path repoPath = m_cache_dir / a_repo.name.GetCString();
    fs::path zipPath = m_cache_dir / (a_repo.name + ".zip").GetCString();

    CURL* curl = curl_easy_init();
    if (!curl) {
      LOG_ERROR("Failed to initialize CURL");
      return false;
    }

    ntl::String url =
        "https://api.github.com/repos/" + a_repo.url + "/zipball/" + a_repo.branch;
    FILE* fp = fopen(zipPath.string().c_str(), "wb");
    if (!fp) {
      curl_easy_cleanup(curl);
      LOG_ERROR("Failed to create zip file");
      return false;
    }

    struct curl_slist* headers = nullptr;
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
      LOG_ERROR(ntl::String{"Download failed: "} + curl_easy_strerror(res));
      return false;
    }

    if (fs::exists(repoPath)) {
      fs::remove_all(repoPath);
    }
    fs::create_directories(repoPath);

    ntl::String cmd = ntl::String{"unzip -o "} + zipPath.string().c_str() + " -d " + repoPath.string().c_str();
    int extract_result = ProcessCommand(cmd, m_log_dir.string().c_str() + ntl::String{"/latest.log"},
                                        m_config.GetCore().verbose);
    fs::remove(zipPath);

    if (extract_result != 0) {
      LOG_ERROR("Failed to extract repository");
      return false;
    }

    fs::path nested_dir;
    for (const auto& entry : fs::directory_iterator(repoPath)) {
      if (fs::is_directory(entry)) {
        nested_dir = entry.path();
        break;
      }
    }

    if (!nested_dir.empty()) {
      for (const auto& entry : fs::directory_iterator(nested_dir)) {
        fs::rename(entry.path(), repoPath / entry.path().filename());
      }
      fs::remove_all(nested_dir);
    }

    return true;
  }

  bool Atlas::removePackage(const PackageConfig& a_config) {
    PackageInstaller installer(m_cache_dir, m_install_dir, m_log_dir, a_config);

    bool success = installer.Uninstall();

    if (!success) {
      LOG_ERROR("Removal failed for " + a_config.name);
      return false;
    }

    return true;
  }

  void Atlas::recordInstallation(const PackageConfig& a_config) {
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
    package["keep"] = false;

    root[a_config.name.GetCString()] = package;

    std::ofstream db_file(db_path);
    db_file << root;
  }

  void Atlas::recordRemoval(const PackageConfig& a_config) {
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

  bool Atlas::upgrade(const PackageConfig& a_config) {
    bool success = true;
    fs::path dbPath = m_install_dir / "installed.json";
    Json::Value root;

    if (fs::exists(dbPath)) {
      std::ifstream dbFile(dbPath);
      dbFile >> root;
    }

    if (IsInstalled(a_config.name)) {
      ntl::String localVersion = root[a_config.name.GetCString()]["version"].asString().c_str();

      if (root[a_config.name.GetCString()]["locked"].asBool()) {
        LOG_MSG("Packege locked for updates " + a_config.name);
        return false;
      }

      if (a_config.version != localVersion) {
        LOG_MSG("Updating " + a_config.name + " from version " + localVersion + " to " + a_config.version + "...");
        success &= Install({a_config.name});
      } else {
        LOG_MSG("No update found");
      }
    }

    return success;
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
        LOG_MSG(ntl::String{"Package '"} + packageName.c_str() + "' is marked to keep and will not be removed.");
        continue;
      }

      LOG_MSG(ntl::String{"Package '"} + packageName.c_str() + "' is not required by any other package.");
      LOG_MSG("Do you want to remove it? (y/n): ");

      char response;
      std::cin >> response;

      if (std::tolower(response) == 'y') {
        if (remove(packageName.c_str())) {
          LOG_MSG(ntl::String{"Successfully removed "} + packageName.c_str());
        } else {
          LOG_MSG(ntl::String{"Failed to remove "} + packageName.c_str());
        }
      }
    }
  }
}
