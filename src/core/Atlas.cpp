//
// Created by Marcus Gugacs on 28.10.24.
//

#include "Atlas.hpp"

#include <set>

#include <toml++/toml.hpp>

#include "utils/Misc.hpp"


static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string *) userp)->append((char *) contents, size * nmemb);
  return size * nmemb;
}

Atlas::Atlas(const fs::path &a_install, const fs::path &a_cache, bool verbose)
  : m_config(),
    m_repo_config_path(m_install_dir / "repositories.json"),
    m_log_dir(m_install_dir / "logs") {

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

bool Atlas::AddRepository(const std::string &a_name, const std::string &a_url, const std::string &a_branch) {
  if (m_repositories.find(a_name) != m_repositories.end()) {
    std::cerr << "Repository already exists\n";
    return false;
  }

  Repository repo{a_name, a_url, a_branch, true};
  m_repositories[a_name] = repo;
  saveRepositories();
  return fetchRepository(repo);
}

bool Atlas::RemoveRepository(const std::string &a_name) {
  if (m_repositories.find(a_name) == m_repositories.end()) {
    std::cerr << "Repository not found\n";
    return false;
  }
  m_repositories.erase(a_name);
  fs::remove_all(m_cache_dir / a_name);
  saveRepositories();
  loadPackageIndex();
  return true;
}

bool Atlas::EnableRepository(const std::string &a_name) {
  if (m_repositories.find(a_name) == m_repositories.end())
    return false;
  m_repositories[a_name].enabled = true;
  saveRepositories();
  loadPackageIndex();
  return true;
}

bool Atlas::DisableRepository(const std::string &a_name) {
  if (m_repositories.find(a_name) == m_repositories.end())
    return false;
  m_repositories[a_name].enabled = false;
  saveRepositories();
  loadPackageIndex();
  return true;
}

void Atlas::ListRepositories() {
  for (const auto &[name, repo]: m_repositories) {
    std::cout << name << " (" << (repo.enabled ? "enabled" : "disabled")
        << ")\n"
        << "  URL: " << repo.url << "\n"
        << "  Branch: " << repo.branch << "\n";
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

    std::cout << "Fetching " << name << "...\n";
    if (!fetchRepository(repo)) {
      std::cerr << "Failed to fetch repository: " << name << "\n";
      success = false;
      continue;
    }

    fs::path repoPath = m_cache_dir / name;
    if (fs::exists(repoPath / "packages.json")) {
      try {
        Json::Value root;
        std::ifstream index_file(repoPath / "packages.json");
        index_file >> root;

        for (const auto &package: root["packages"]) {
          PackageConfig config{
            package["name"].asString(),
            package["version"].asString(),
            package["description"].asString(),
            package["build_command"].asString(),
            package["install_command"].asString(),
            package["uninstall_command"].asString(),
            name
          };

          const Json::Value &deps = package["dependencies"];
          for (const auto &dep: deps) {
            config.dependencies.push_back(dep.asString());
          }
          m_package_index[config.name] = config;
        }
      } catch (const std::exception &e) {
        std::cerr << "Error parsing package index for " << name << ": "
            << e.what() << "\n";
        success = false;
      }
    }
  }

  fs::remove_all(tempDir);
  loadingAnimation.Stop();
  return success;
}

bool Atlas::Install(const std::string &a_package_name) {
  if (m_package_index.find(a_package_name) == m_package_index.end()) {
    std::cerr << "Package not found\n";
    return false;
  }
  return installPackage(m_package_index[a_package_name]);
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
      std::string localVersion = root[name]["version"].asString();
      if (config.version != localVersion) {
        installedSomething = true;
        std::cout << "Updating " << name << " from version " << localVersion
            << " to " << config.version << "...\n";
        success &= installPackage(config);
      }
    }
  }

  if (!installedSomething) {
    std::cout << "No updates found\n";
  }

  return success;
}

bool Atlas::Remove(const std::string &a_package_name) {
  if (m_package_index.find(a_package_name) == m_package_index.end()) {
    std::cerr << "Package not found\n";
    return false;
  }
  return removePackage(m_package_index[a_package_name]);
}

void Atlas::Cleanup() {
  std::cout << "Finding orphaned packages to remove...\n";
  cleanupPackages();
}

std::vector<std::string> Atlas::Search(const std::string &a_query) {
  std::vector<std::string> results;
  for (const auto &[name, config]: m_package_index) {
    if (name.find(a_query) != std::string::npos ||
        config.description.find(a_query) != std::string::npos) {
      results.push_back(name);
    }
  }
  return results;
}

void Atlas::Info(const std::string &a_package_name) {
  if (m_package_index.find(a_package_name) == m_package_index.end()) {
    std::cerr << "Package not found\n";
    return;
  }

  const auto &config = m_package_index[a_package_name];
  std::cout << "Name: " << config.name << "\n"
      << "Version: " << config.version << "\n"
      << "Description: " << config.description << "\n"
      << "Status: " << (IsInstalled(a_package_name) ? GREEN : RED)
      << (IsInstalled(a_package_name) ? "Installed" : "Not installed")
      << "\n";
}

bool Atlas::IsInstalled(const std::string &a_package_name) const {
  fs::path dbPath = m_install_dir / "installed.json";
  if (!fs::exists(dbPath)) {
    return false;
  }

  Json::Value root;
  std::ifstream dbFile(dbPath);
  dbFile >> root;

  return root.isMember(a_package_name);
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
      repo["name"].asString(), repo["url"].asString(),
      repo["branch"].asString(), repo["enabled"].asBool()
    };
    m_repositories[r.name] = r;
  }
}

void Atlas::saveRepositories() {
  Json::Value root;
  Json::Value repoArray(Json::arrayValue);

  for (const auto &[name, repo]: m_repositories) {
    Json::Value repoObj;
    repoObj["name"] = repo.name;
    repoObj["url"] = repo.url;
    repoObj["branch"] = repo.branch;
    repoObj["enabled"] = repo.enabled;
    repoArray.append(repoObj);
  }
  root["repositories"] = repoArray;

  std::ofstream config_file(m_repo_config_path);
  config_file << root;
}

void Atlas::loadPackageIndex() {
  m_package_index.clear();
  for (const auto &[name, repo]: m_repositories) {
    if (!repo.enabled)
      continue;

    fs::path repoPath = m_cache_dir / name;
    for (const auto &entry: fs::recursive_directory_iterator(repoPath)) {
      if (entry.path().filename() == "package.json") {
        Json::Value root;
        std::ifstream config_file(entry.path());
        config_file >> root;

        PackageConfig config{
          root["name"].asString(),
          root["version"].asString(),
          root["description"].asString(),
          root["build_command"].asString(),
          root["install_command"].asString(),
          root["uninstall_command"].asString(),
          name
        };
        m_package_index[config.name] = config;
      }
    }
  }
}

bool Atlas::fetchRepository(const Repository &a_repo) const {
  fs::path repoPath = m_cache_dir / a_repo.name;
  fs::path zipPath = m_cache_dir / (a_repo.name + ".zip");

  CURL *curl = curl_easy_init();
  if (!curl) {
    std::cerr << "Failed to initialize CURL\n";
    return false;
  }

  std::string url =
      "https://api.github.com/repos/" + a_repo.url + "/zipball/" + a_repo.branch;
  FILE *fp = fopen(zipPath.string().c_str(), "wb");
  if (!fp) {
    curl_easy_cleanup(curl);
    std::cerr << "Failed to create zip file\n";
    return false;
  }

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "Accept: application/vnd.github+json");
  headers = curl_slist_append(headers, "User-Agent: Atlas-Package-Manager");

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, nullptr);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  CURLcode res = curl_easy_perform(curl);
  fclose(fp);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    std::cerr << "Download failed: " << curl_easy_strerror(res) << "\n";
    return false;
  }

  if (fs::exists(repoPath)) {
    fs::remove_all(repoPath);
  }
  fs::create_directories(repoPath);

  std::string cmd =
      "unzip -o " + zipPath.string() + " -d " + repoPath.string();
  int extract_result =
      ProcessCommand(cmd, m_log_dir.string() + "/latest.log", m_config.GetCore().verbose);
  fs::remove(zipPath);

  if (extract_result != 0) {
    std::cerr << "Failed to extract repository\n";
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
    if (m_package_index.find(dep) == m_package_index.end()) {
      std::cerr << "Unknown dependency " << dep << "\n";
      return false;
    }

    PackageConfig dep_config = m_package_index[dep];
    if (!installPackage(dep_config)) {
      std::cerr << "Failed to install dependency: " + dep + "\n";
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
    std::cerr << "Installation failed for " + a_config.name + "\n";
    return false;
  }

  recordInstallation(a_config);
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
  package["version"] = a_config.version;
  package["install_date"] = getCurrentDateTime();
  package["repository"] = a_config.repository;

  root[a_config.name] = package;

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

  root.removeMember(a_config.name);

  std::ofstream db_file(db_path);
  db_file << root;
}

std::string Atlas::getCurrentDateTime() {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::string datetime = std::ctime(&time);
  datetime.pop_back();
  return datetime;
}

bool Atlas::removePackage(const PackageConfig &a_config) {
  PackageInstaller installer(m_cache_dir, m_install_dir, m_log_dir, a_config);

  LoadingAnimation loading("Removing " + a_config.name);

  bool success = installer.Uninstall();

  loading.Stop();

  if (!success) {
    std::cerr << "Removal failed for " + a_config.name + "\n";
    return false;
  }

  recordRemoval(a_config);

  return true;
}

bool Atlas::isMacOS() {
#ifdef __APPLE__
  return true;
#else
        return false;
#endif
}

fs::path Atlas::getDefaultShortcutDir() {
  fs::path homeDir = getenv("HOME");
  return isMacOS()
           ? homeDir / "Applications"
           : homeDir / ".local/share/applications";
}

void Atlas::createShortcut(const std::string &a_repo) {
  if (isMacOS()) {
    createMacOSShortcut(a_repo);
  } else {
    createLinuxShortcut(a_repo);
  }
}

void Atlas::createMacOSShortcut(const std::string &a_repo) {
  fs::path appPath = m_shortcut_dir / (a_repo + ".app");
  fs::create_directories(appPath / "Contents/MacOS");

  std::ofstream plist(appPath / "Contents/Info.plist");
  plist << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
      "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
      << "<plist version=\"1.0\">\n"
      << "<dict>\n"
      << "    <key>CFBundleExecutable</key>\n"
      << "    <string>" << a_repo << "</string>\n"
      << "    <key>CFBundleIdentifier</key>\n"
      << "    <string>com.atlas." << a_repo << "</string>\n"
      << "    <key>CFBundleName</key>\n"
      << "    <string>" << a_repo << "</string>\n"
      << "    <key>CFBundlePackageType</key>\n"
      << "    <string>APPL</string>\n"
      << "    <key>CFBundleShortVersionString</key>\n"
      << "    <string>1.0</string>\n"
      << "</dict>\n"
      << "</plist>";
  plist.close();

  fs::create_symlink(m_install_dir / a_repo / "main",
                     appPath / "Contents/MacOS" / a_repo);
}

void Atlas::createLinuxShortcut(const std::string &a_repo) {
  fs::path shortcutPath = m_shortcut_dir / (a_repo + ".desktop");
  std::ofstream shortcut(shortcutPath);

  shortcut << "[Desktop Entry]\n"
      << "Name=" << a_repo << "\n"
      << "Exec=" << (m_install_dir / a_repo / "main").string() << "\n"
      << "Type=Application\n"
      << "Terminal=false\n";

  shortcut.close();
  fs::permissions(shortcutPath, fs::perms::owner_all | fs::perms::group_read |
                                fs::perms::others_read);
}

bool Atlas::downloadRepository(const std::string &a_username, const std::string &a_repo) const {
  CURL *curl = curl_easy_init();
  std::string url = "https://api.github.com/repos/" + a_username + "/" + a_repo +
                    "/zipball/master";
  std::string zipPath = (m_cache_dir / (a_repo + ".zip")).string();

  if (!curl)
    return false;

  FILE *fp = fopen(zipPath.c_str(), "wb");
  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "Accept: application/vnd.github+json");
  headers = curl_slist_append(headers, "User-Agent: Atlas-Package-Manager");

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
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

bool Atlas::extractPackage(const std::string &a_repo) const {
  std::string zipPath = (m_cache_dir / (a_repo + ".zip")).string();
  std::string extractPath = (m_install_dir / a_repo).string();
  std::string cmd = "unzip -o " + zipPath + " -d " + extractPath;
  return system(cmd.c_str()) == 0;
}

void Atlas::cleanupPackages() {
  // Read installed packages
  fs::path dbPath = m_install_dir / "installed.json";
  if (!fs::exists(dbPath)) {
    return;
  }

  Json::Value root;
  std::ifstream dbFile(dbPath);
  dbFile >> root;

  // Create a set of all dependencies
  std::set<std::string> allDependencies;
  for (const auto& packageName : root.getMemberNames()) {
    const auto& deps = root[packageName]["dependencies"];
    for (const auto& dep : deps) {
      allDependencies.insert(dep.asString());
    }
  }

  // Check each installed package
  for (const auto& packageName : root.getMemberNames()) {
    // Skip if package is a dependency of another package
    if (allDependencies.contains(packageName)) {
      continue;
    }

    std::cout << "Package '" << packageName << "' is not required by any other package.\n";
    std::cout << "Do you want to remove it? (y/n): ";

    char response;
    std::cin >> response;

    if (std::tolower(response) == 'y') {
      if (remove(packageName.c_str())) {
        std::cout << "Successfully removed " << packageName << "\n";
      } else {
        std::cout << "Failed to remove " << packageName << "\n";
      }
    }
  }
}
