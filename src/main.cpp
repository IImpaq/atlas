#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <filesystem>
#include <exception>
#include <cstdlib>

#include "core/Atlas.hpp"
#include "utils/Misc.hpp"

namespace fs = std::filesystem;

using namespace atlas;

bool hasVerboseFlag(int argc, char* argv[]) {
  for (int i = 1; i < argc; i++) {
    ntl::String arg = argv[i];
    if (arg == "-v" || arg == "--verbose") {
      std::cout << "Verbose mode enabled\n";
      return true;
    }
  }
  return false;
}

struct Command {
  ntl::String description;
  int requiredArgs;
  std::function<bool(atlas::Atlas&, const ntl::Array<ntl::String>&)> handler;
};

void printHelp(const char* progName) {
  std::cout << "\n🔧 " << CYAN << progName << RESET << " - Package Manager\n\n"
      << YELLOW << "Usage:" << RESET << " " << progName << " <command> [args]\n\n"
      << YELLOW << "Repository Management:" << RESET << "\n"
      << "  repo-add <name> <url>      Add a new repository\n"
      << "  repo-remove <name>         Remove a repository\n"
      << "  repo-enable <name>         Enable a repository\n"
      << "  repo-disable <name>        Disable a repository\n"
      << "  repo-list                  List all repositories\n\n"
      << YELLOW << "Package Operations:" << RESET << "\n"
      << "  install <package>          Install a package\n"
      << "  remove <package>           Remove a package\n"
      << "  update                     Update all packages\n"
      << "  upgrade <package>          Upgrade specific package\n"
      << "  search <query>             Search for packages\n"
      << "  info <package>             Show package details\n\n"
      << YELLOW << "Package Management:" << RESET << "\n"
      << "  lock <package>             Prevent package updates\n"
      << "  unlock <package>           Allow package updates\n"
      << "  keep <package>             Protect from cleanup\n"
      << "  unkeep <package>           Allow cleanup\n"
      << "  cleanup                    Remove unused packages\n\n"
      << YELLOW << "Atlas Management:" << RESET << "\n"
      << "  self-setup                 Setup atlas to be globally accessible\n"
      << "  self-purge                 Get rid of atlas again\n\n"
      << YELLOW << "Options:" << RESET << "\n"
      << "  -v, --verbose              Enable verbose output\n";
}

const std::map<ntl::String, Command> COMMANDS = {
  {
    "repo-add", {
      "Add a new repository", 2,
      [](atlas::Atlas& pm, const auto& args) { return pm.AddRepository(args[0], args[1]); }
    }
  },
  {
    "repo-remove", {
      "Remove a repository", 1,
      [](atlas::Atlas& pm, const auto& args) { return pm.RemoveRepository(args[0]); }
    }
  },
  {
    "repo-enable", {
      "Enable a repository", 1,
      [](atlas::Atlas& pm, const auto& args) { return pm.EnableRepository(args[0]); }
    }
  },
  {
    "repo-disable", {
      "Disable a repository", 1,
      [](atlas::Atlas& pm, const auto& args) { return pm.DisableRepository(args[0]); }
    }
  },
  {
    "repo-list", {
      "List all repositories", 0,
      [](atlas::Atlas& pm, const auto&) {
        pm.ListRepositories();
        return true;
      }
    }
  },
  {
    "fetch", {
      "Fetch updates", 0,
      [](atlas::Atlas& pm, const auto&) { return pm.Fetch(); }
    }
  },
  {
    "install", {
      "Install a package", -1,
      [](atlas::Atlas& pm, const auto& args) { return pm.Install(args); }
    }
  },
  {
    "remove", {
      "Remove a package", 1,
      [](atlas::Atlas& pm, const auto& args) { return pm.Remove(args[0]); }
    }
  },
  {
    "update", {
      "Update all packages", 0,
      [](atlas::Atlas& pm, const auto&) { return pm.Update(); }
    }
  },
  {
    "upgrade", {
      "Upgrade a package", 1,
      [](atlas::Atlas& pm, const auto& args) { return pm.Upgrade(args[0]); }
    }
  },
  {
    "lock", {
      "Lock a package version", 1,
      [](atlas::Atlas& pm, const auto& args) { return pm.LockPackage(args[0]); }
    }
  },
  {
    "unlock", {
      "Unlock a package version", 1,
      [](atlas::Atlas& pm, const auto& args) { return pm.UnlockPackage(args[0]); }
    }
  },
  {
    "cleanup", {
      "Clean unused packages", 0,
      [](atlas::Atlas& pm, const auto&) {
        pm.Cleanup();
        return true;
      }
    }
  },
  {
    "keep", {
      "Keep a package", 1,
      [](atlas::Atlas& pm, const auto& args) { return pm.KeepPackage(args[0]); }
    }
  },
  {
    "unkeep", {
      "Unkeep a package", 1,
      [](atlas::Atlas& pm, const auto& args) { return pm.UnkeepPackage(args[0]); }
    }
  },
  {
    "search", {
      "Search for packages", 1,
      [](atlas::Atlas& pm, const auto& args) {
        auto results = pm.Search(args[0]);
        for (const auto& result : results) std::cout << result << "\n";
        return true;
      }
    }
  },
  {
    "info", {
      "Show package information", 1,
      [](atlas::Atlas& pm, const auto& args) {
        pm.Info(args[0]);
        return true;
      }
    }
  },
  {
    "self-setup", {
      "Setup atlas to be globally accessible", 0,
      [](atlas::Atlas& pm, const auto& args) { return pm.AtlasSetup(); }
    }
  },
  {
    "self-purge", {
      "Get rid of atlas again", 0,
      [](atlas::Atlas& pm, const auto& args) { return pm.AtlasPurge(); }
    }
  }
};

int main(int argc, char* argv[]) {
  const char* homeDir = getenv("HOME");

  atlas::Atlas pm(fs::path(homeDir) / ".local/share/atlas",
                  fs::path(homeDir) / ".cache/atlas",
                  hasVerboseFlag(argc, argv));

  if (!homeDir) {
    LOG_ERROR("HOME environment variable not set");
    return 1;
  }

  if (argc < 2) {
    LOG_ERROR("No command specified");
    printHelp(argv[0]);
    return 1;
  }

  ntl::String command = argv[1];

  if (command == "help" || command == "--help" || command == "-h") {
    printHelp(argv[0]);
    return 0;
  }

  auto cmdIt = COMMANDS.find(command);
  if (cmdIt == COMMANDS.end()) {
    LOG_ERROR("Unknown command '" + command + "'");
    printHelp(argv[0]);
    return 1;
  }

  const Command& cmd = cmdIt->second;
  int providedArgs = argc - 2;

  if (cmd.requiredArgs != -1 && providedArgs != cmd.requiredArgs) {
    LOG_ERROR("'" + command + ntl::String("' requires ") + cmd.requiredArgs + " argument(s)");
    return 1;
  }

  try {
    ntl::Array<ntl::String> args{};
    for (int i = 2; i < argc; i++) {
      args.Insert(argv[i]);
    }

    bool result = cmd.handler(pm, args);

    if (result) {
      LOG_INFO("Command completed successfully");
      return 0;
    } else {
      LOG_ERROR("Command failed");
      return 1;
    }
  } catch (const std::exception& e) {
    LOG_ERROR(e.what());
    return 1;
  }
}
