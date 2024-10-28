#include <cstdlib>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <json/json.h>
#include <regex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

const char *RED = "\033[31m";
const char *GREEN = "\033[32m";
const char *YELLOW = "\033[33m";
const char *RESET = "\033[0m";

struct Repository {
  std::string name;
  std::string url;
  std::string branch;
  bool enabled;
};

struct PackageConfig {
  std::string name;
  std::string version;
  std::string description;
  std::string build_command;
  std::string install_command;
  std::string uninstall_command;
  std::string repository;
  std::vector<std::string> dependencies;
};

class LoadingAnimation {
private:
  bool running;
  std::thread animator;
  std::string message;
  const std::vector<std::string> frames = {"|", "/", "-", "\\"};

  void animate() {
    int frame = 0;
    while (running) {
      std::cout << "\r" << message << frames[frame] << std::flush;
      frame = (frame + 1) % frames.size();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "\r" << message << " ✓" << std::endl;
  }

public:
  explicit LoadingAnimation(const std::string &msg)
      : running(true), message(msg) {
    animator = std::thread(&LoadingAnimation::animate, this);
  }

  ~LoadingAnimation() { stop(); }

  void stop() {
    if (running) {
      running = false;
      if (animator.joinable()) {
        animator.join();
      }
    }
  }
};

static void logOutputToFile(const std::string &output,
                            const std::string &filename) {
  std::ofstream file(filename, std::ios::app);
  file << output;
  file.close();
}

static int processCommand(const std::string &command, const std::string &path,
                          bool verbose) {
  FILE *pipe = popen(command.c_str(), "r");
  std::string output{};
  int exitCode = -1;
  if (pipe) {
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
      if (verbose)
        std::cout << "Debug: " << buffer << std::endl;
      output.append(buffer);
    }
    exitCode = pclose(pipe);
  }
  logOutputToFile(output, path);
  return exitCode;
}

class PackageInstaller {
private:
  fs::path cacheDir;
  fs::path installDir;
  fs::path logDir;
  Json::Value config;
  std::string platform;

  bool executeCommands(const Json::Value &commands) {
    for (const auto &cmd : commands) {
      std::string command = replaceVariables(cmd.asString());
      if (processCommand(command, logDir / "latest.log", false) != 0) {
        return false;
      }
    }
    return true;
  }

  std::string replaceVariables(const std::string &cmd) {
    std::string result = cmd;
    result = std::regex_replace(result, std::regex("\\$PACKAGE_CACHE_DIR"),
                                cacheDir.string());
    result = std::regex_replace(result, std::regex("\\$INSTALL_DIR"),
                                installDir.string());
    return result;
  }

  bool downloadFile(const std::string &url, const std::string &target) {
    std::string targetPath = replaceVariables(target);
    CURL *curl = curl_easy_init();
    if (!curl)
      return false;

    FILE *fp = fopen(targetPath.c_str(), "wb");
    if (!fp) {
      curl_easy_cleanup(curl);
      return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);

    return res == CURLE_OK;
  }

public:
  PackageInstaller(const fs::path &cache, const fs::path &install,
                   const fs::path &log, const PackageConfig &packageConfig)
      : cacheDir(cache), installDir(install), logDir(log) {
#ifdef __APPLE__
    platform = "macos";
#else
    platform = "linux";
#endif

    // Load package.json from the package's directory
    fs::path packageJsonPath = cacheDir / packageConfig.repository /
                               "packages" / packageConfig.name / "package.json";
    std::ifstream configFile(packageJsonPath);
    if (configFile.is_open()) {
      configFile >> config;
    }
  }

  bool download() {
    const auto &step = config["platforms"][platform]["steps"]["download"];
    return downloadFile(step["url"].asString(), step["target"].asString());
  }

  bool prepare() {
    return executeCommands(
        config["platforms"][platform]["steps"]["prepare"]["commands"]);
  }

  bool build() {
    return executeCommands(
        config["platforms"][platform]["steps"]["build"]["commands"]);
  }

  bool install() {
    return executeCommands(
        config["platforms"][platform]["steps"]["install"]["commands"]);
  }

  bool cleanup() {
    return executeCommands(
        config["platforms"][platform]["steps"]["cleanup"]["commands"]);
  }

  bool uninstall() {
    return executeCommands(
        config["platforms"][platform]["steps"]["uninstall"]["commands"]);
  }
};

class PackageManager {
private:
  bool verbose;
  fs::path installDir;
  fs::path cacheDir;
  fs::path shortcutDir;
  fs::path repoConfigPath;
  fs::path logDir;
  std::unordered_map<std::string, Repository> repositories;
  std::unordered_map<std::string, PackageConfig> packageIndex;

  void loadRepositories() {
    if (!fs::exists(repoConfigPath)) {
      return;
    }

    Json::Value root;
    std::ifstream config_file(repoConfigPath);
    config_file >> root;

    for (const auto &repo : root["repositories"]) {
      Repository r{repo["name"].asString(), repo["url"].asString(),
                   repo["branch"].asString(), repo["enabled"].asBool()};
      repositories[r.name] = r;
    }
  }

  void saveRepositories() {
    Json::Value root;
    Json::Value repoArray(Json::arrayValue);

    for (const auto &[name, repo] : repositories) {
      Json::Value repoObj;
      repoObj["name"] = repo.name;
      repoObj["url"] = repo.url;
      repoObj["branch"] = repo.branch;
      repoObj["enabled"] = repo.enabled;
      repoArray.append(repoObj);
    }
    root["repositories"] = repoArray;

    std::ofstream config_file(repoConfigPath);
    config_file << root;
  }

  void loadPackageIndex() {
    packageIndex.clear();
    for (const auto &[name, repo] : repositories) {
      if (!repo.enabled)
        continue;

      fs::path repoPath = cacheDir / name;
      for (const auto &entry : fs::recursive_directory_iterator(repoPath)) {
        if (entry.path().filename() == "package.json") {
          Json::Value root;
          std::ifstream config_file(entry.path());
          config_file >> root;

          PackageConfig config{root["name"].asString(),
                               root["version"].asString(),
                               root["description"].asString(),
                               root["build_command"].asString(),
                               root["install_command"].asString(),
                               root["uninstall_command"].asString(),
                               name};
          packageIndex[config.name] = config;
        }
      }
    }
  }

  bool fetchRepository(const Repository &repo) {
    fs::path repoPath = cacheDir / repo.name;
    fs::path zipPath = cacheDir / (repo.name + ".zip");

    CURL *curl = curl_easy_init();
    if (!curl) {
      std::cerr << "Failed to initialize CURL\n";
      return false;
    }

    std::string url =
        "https://api.github.com/repos/" + repo.url + "/zipball/" + repo.branch;
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
        processCommand(cmd, logDir.string() + "/latest.log", verbose);
    fs::remove(zipPath);

    if (extract_result != 0) {
      std::cerr << "Failed to extract repository\n";
      return false;
    }

    fs::path nested_dir;
    for (const auto &entry : fs::directory_iterator(repoPath)) {
      if (fs::is_directory(entry)) {
        nested_dir = entry.path();
        break;
      }
    }

    if (!nested_dir.empty()) {
      for (const auto &entry : fs::directory_iterator(nested_dir)) {
        fs::rename(entry.path(), repoPath / entry.path().filename());
      }
      fs::remove_all(nested_dir);
    }

    return true;
  }

  bool installPackage(const PackageConfig &config) {
    PackageInstaller installer(cacheDir, installDir, logDir, config);

    LoadingAnimation loading("Installing " + config.name);

    bool success = installer.download() && installer.prepare() &&
                   installer.build() && installer.install() &&
                   installer.cleanup();

    loading.stop();

    if (!success) {
      std::cerr << "Installation failed for " + config.name + "\n";
      return false;
    }

    recordInstallation(config);
    return true;
  }

  void recordInstallation(const PackageConfig &config) {
    Json::Value root;
    fs::path db_path = installDir / "installed.json";

    if (fs::exists(db_path)) {
      std::ifstream db_file(db_path);
      db_file >> root;
    }

    Json::Value package;
    package["version"] = config.version;
    package["install_date"] = getCurrentDateTime();
    package["repository"] = config.repository;

    root[config.name] = package;

    std::ofstream db_file(db_path);
    db_file << root;
  }

  void recordRemoval(const PackageConfig &config) {
    Json::Value root;
    fs::path db_path = installDir / "installed.json";

    if (fs::exists(db_path)) {
      std::ifstream db_file(db_path);
      db_file >> root;
    }

    root.removeMember(config.name);

    std::ofstream db_file(db_path);
    db_file << root;
  }

  std::string getCurrentDateTime() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::string datetime = std::ctime(&time);
    datetime.pop_back();
    return datetime;
  }

  bool removePackage(const PackageConfig &config) {
    PackageInstaller installer(cacheDir, installDir, logDir, config);

    LoadingAnimation loading("Removing " + config.name);

    bool success = installer.uninstall();

    loading.stop();

    if (!success) {
      std::cerr << "Removal failed for " + config.name + "\n";
      return false;
    }

    recordRemoval(config);

    return true;
  }

  bool isMacOS() {
#ifdef __APPLE__
    return true;
#else
    return false;
#endif
  }

  fs::path getDefaultShortcutDir() {
    fs::path homeDir = getenv("HOME");
    return isMacOS() ? homeDir / "Applications"
                     : homeDir / ".local/share/applications";
  }

  void createShortcut(const std::string &repo) {
    if (isMacOS()) {
      createMacOSShortcut(repo);
    } else {
      createLinuxShortcut(repo);
    }
  }

  void createMacOSShortcut(const std::string &repo) {
    fs::path appPath = shortcutDir / (repo + ".app");
    fs::create_directories(appPath / "Contents/MacOS");

    std::ofstream plist(appPath / "Contents/Info.plist");
    plist << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
             "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
          << "<plist version=\"1.0\">\n"
          << "<dict>\n"
          << "    <key>CFBundleExecutable</key>\n"
          << "    <string>" << repo << "</string>\n"
          << "    <key>CFBundleIdentifier</key>\n"
          << "    <string>com.atlas." << repo << "</string>\n"
          << "    <key>CFBundleName</key>\n"
          << "    <string>" << repo << "</string>\n"
          << "    <key>CFBundlePackageType</key>\n"
          << "    <string>APPL</string>\n"
          << "    <key>CFBundleShortVersionString</key>\n"
          << "    <string>1.0</string>\n"
          << "</dict>\n"
          << "</plist>";
    plist.close();

    fs::create_symlink(installDir / repo / "main",
                       appPath / "Contents/MacOS" / repo);
  }

  void createLinuxShortcut(const std::string &repo) {
    fs::path shortcutPath = shortcutDir / (repo + ".desktop");
    std::ofstream shortcut(shortcutPath);

    shortcut << "[Desktop Entry]\n"
             << "Name=" << repo << "\n"
             << "Exec=" << (installDir / repo / "main").string() << "\n"
             << "Type=Application\n"
             << "Terminal=false\n";

    shortcut.close();
    fs::permissions(shortcutPath, fs::perms::owner_all | fs::perms::group_read |
                                      fs::perms::others_read);
  }

  static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                              void *userp) {
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
  }

  bool downloadRepository(const std::string &username,
                          const std::string &repo) {
    CURL *curl = curl_easy_init();
    std::string url = "https://api.github.com/repos/" + username + "/" + repo +
                      "/zipball/master";
    std::string zipPath = (cacheDir / (repo + ".zip")).string();

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

  bool extractPackage(const std::string &repo) {
    std::string zipPath = (cacheDir / (repo + ".zip")).string();
    std::string extractPath = (installDir / repo).string();
    std::string cmd = "unzip -o " + zipPath + " -d " + extractPath;
    return system(cmd.c_str()) == 0;
  }

public:
  PackageManager(const fs::path &install, const fs::path &cache, bool verbose)
      : installDir(fs::path(getenv("HOME")) / ".local/share/atlas"),
        cacheDir(fs::path(getenv("HOME")) / ".cache/atlas"),
        shortcutDir(getDefaultShortcutDir()),
        repoConfigPath(installDir / "repositories.json"),
        logDir(installDir / "logs"), verbose(verbose) {
    fs::create_directories(installDir);
    fs::create_directories(cacheDir);
    fs::create_directories(shortcutDir);
    fs::create_directories(logDir);
    loadRepositories();
    loadPackageIndex();
  }

  bool addRepository(const std::string &name, const std::string &url,
                     const std::string &branch = "main") {
    if (repositories.find(name) != repositories.end()) {
      std::cerr << "Repository already exists\n";
      return false;
    }

    Repository repo{name, url, branch, true};
    repositories[name] = repo;
    saveRepositories();
    return fetchRepository(repo);
  }

  bool removeRepository(const std::string &name) {
    if (repositories.find(name) == repositories.end()) {
      std::cerr << "Repository not found\n";
      return false;
    }
    repositories.erase(name);
    fs::remove_all(cacheDir / name);
    saveRepositories();
    loadPackageIndex();
    return true;
  }

  bool enableRepository(const std::string &name) {
    if (repositories.find(name) == repositories.end())
      return false;
    repositories[name].enabled = true;
    saveRepositories();
    loadPackageIndex();
    return true;
  }

  bool disableRepository(const std::string &name) {
    if (repositories.find(name) == repositories.end())
      return false;
    repositories[name].enabled = false;
    saveRepositories();
    loadPackageIndex();
    return true;
  }

  void listRepositories() {
    for (const auto &[name, repo] : repositories) {
      std::cout << name << " (" << (repo.enabled ? "enabled" : "disabled")
                << ")\n"
                << "  URL: " << repo.url << "\n"
                << "  Branch: " << repo.branch << "\n";
    }
  }

  bool fetch() {
    bool success = true;
    LoadingAnimation loadingAnimation("Fetching repositories");
    fs::path tempDir = cacheDir / "temp";
    fs::create_directories(tempDir);

    for (const auto &[name, repo] : repositories) {
      if (!repo.enabled)
        continue;

      std::cout << "Fetching " << name << "...\n";
      if (!fetchRepository(repo)) {
        std::cerr << "Failed to fetch repository: " << name << "\n";
        success = false;
        continue;
      }

      fs::path repoPath = cacheDir / name;
      if (fs::exists(repoPath / "packages.json")) {
        try {
          Json::Value root;
          std::ifstream index_file(repoPath / "packages.json");
          index_file >> root;

          for (const auto &package : root["packages"]) {
            PackageConfig config{package["name"].asString(),
                                 package["version"].asString(),
                                 package["description"].asString(),
                                 package["build_command"].asString(),
                                 package["install_command"].asString(),
                                 package["uninstall_command"].asString(),
                                 name};

            const Json::Value &deps = package["dependencies"];
            for (const auto &dep : deps) {
              config.dependencies.push_back(dep.asString());
            }
            packageIndex[config.name] = config;
          }
        } catch (const std::exception &e) {
          std::cerr << "Error parsing package index for " << name << ": "
                    << e.what() << "\n";
          success = false;
        }
      }
    }

    fs::remove_all(tempDir);
    loadingAnimation.stop();
    return success;
  }

  bool install(const std::string &packageName) {
    if (packageIndex.find(packageName) == packageIndex.end()) {
      std::cerr << "Package not found\n";
      return false;
    }
    return installPackage(packageIndex[packageName]);
  }

  bool update() {
    bool success = true;
    fs::path dbPath = installDir / "installed.json";
    Json::Value root;

    if (fs::exists(dbPath)) {
      std::ifstream dbFile(dbPath);
      dbFile >> root;
    }

    bool installedSomething = false;

    for (const auto &[name, config] : packageIndex) {
      if (isInstalled(name)) {
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

  bool remove(const std::string &packageName) {
    if (packageIndex.find(packageName) == packageIndex.end()) {
      std::cerr << "Package not found\n";
      return false;
    }
    return removePackage(packageIndex[packageName]);
  }

  std::vector<std::string> search(const std::string &query) {
    std::vector<std::string> results;
    for (const auto &[name, config] : packageIndex) {
      if (name.find(query) != std::string::npos ||
          config.description.find(query) != std::string::npos) {
        results.push_back(name);
      }
    }
    return results;
  }

  void info(const std::string &packageName) {
    if (packageIndex.find(packageName) == packageIndex.end()) {
      std::cerr << "Package not found\n";
      return;
    }

    const auto &config = packageIndex[packageName];
    std::cout << "Name: " << config.name << "\n"
              << "Version: " << config.version << "\n"
              << "Description: " << config.description << "\n"
              << "Status: " << (isInstalled(packageName) ? GREEN : RED)
              << (isInstalled(packageName) ? "Installed" : "Not installed")
              << "\n";
  }

  bool isInstalled(const std::string &packageName) {
    fs::path dbPath = installDir / "installed.json";
    if (!fs::exists(dbPath)) {
      return false;
    }

    Json::Value root;
    std::ifstream dbFile(dbPath);
    dbFile >> root;

    return root.isMember(packageName);
  }
};

void printHelp(const char *progName) {
  std::cout << YELLOW << "Usage: " << progName << " <command> [args]\n\n"
            << "Available commands:\n"
            << GREEN << "  repo-add <name> <url>     Add a new repository\n"
            << "  repo-remove <name>        Remove a repository\n"
            << "  repo-enable <name>        Enable a repository\n"
            << "  repo-disable <name>       Disable a repository\n"
            << "  repo-list                 List all repositories\n"
            << "  fetch                     Fetch updates from repositories\n"
            << "  install <package>         Install a package\n"
            << "  update                    Update installed packages\n"
            << "  remove <package>          Remove a package\n"
            << "  search <query>            Search for packages\n"
            << "  info <package>            Show package information\n"
            << "  help                      Show this help message\n"
            << RESET;
}

bool hasVerboseFlag(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "-v" || arg == "--verbose") {
      std::cout << "Verbose mode enabled\n";
      return true;
    }
  }
  return false;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << RED << "Error: No command specified\n" << RESET;
    printHelp(argv[0]);
    return 1;
  }

  fs::path homeDir = getenv("HOME");
  PackageManager pm(homeDir / ".local/share/atlas", homeDir / ".cache/atlas",
                    hasVerboseFlag(argc, argv));

  std::string command = argv[1];

  if (command == "help") {
    printHelp(argv[0]);
    return 0;
  }

  if (command == "repo-add") {
    if (argc != 4) {
      std::cout << RED << "Error: repo-add requires name and URL arguments\n"
                << RESET;
      return 1;
    }
    return pm.addRepository(argv[2], argv[3]) ? 0 : 1;
  }

  if (command == "repo-remove") {
    if (argc != 3) {
      std::cout << RED << "Error: repo-remove requires repository name\n"
                << RESET;
      return 1;
    }
    return pm.removeRepository(argv[2]) ? 0 : 1;
  }

  if (command == "repo-enable") {
    if (argc != 3) {
      std::cout << RED << "Error: repo-enable requires repository name\n"
                << RESET;
      return 1;
    }
    return pm.enableRepository(argv[2]) ? 0 : 1;
  }

  if (command == "repo-disable") {
    if (argc != 3) {
      std::cout << RED << "Error: repo-disable requires repository name\n"
                << RESET;
      return 1;
    }
    return pm.disableRepository(argv[2]) ? 0 : 1;
  }

  if (command == "repo-list") {
    pm.listRepositories();
    return 0;
  }

  if (command == "fetch") {
    return pm.fetch() ? 0 : 1;
  }

  if (command == "install") {
    if (argc != 3) {
      std::cout << RED << "Error: install requires package name\n" << RESET;
      return 1;
    }
    return pm.install(argv[2]) ? 0 : 1;
  }

  if (command == "update") {
    return pm.update() ? 0 : 1;
  }

  if (command == "remove") {
    if (argc != 3) {
      std::cout << RED << "Error: remove requires package name\n" << RESET;
      return 1;
    }
    return pm.remove(argv[2]) ? 0 : 1;
  }

  if (command == "search") {
    if (argc != 3) {
      std::cout << RED << "Error: search requires a query argument\n" << RESET;
      return 1;
    }
    for (const auto &result : pm.search(argv[2]))
      std::cout << result << "\n";
    return 0;
  }

  if (command == "info") {
    if (argc != 3) {
      std::cout << RED << "Error: info requires package name\n" << RESET;
      return 1;
    }
    pm.info(argv[2]);
    return 0;
  }

  std::cout << RED << "Error: Invalid command '" << command << "'\n" << RESET;
  printHelp(argv[0]);
  return 1;
}
