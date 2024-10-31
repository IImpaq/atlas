#include <iostream>
#include <string>
#include <fstream>

#include "core/Atlas.hpp"
#include "utils/Misc.hpp"

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

void printHelp(const char *progName) {
    std::cout << YELLOW << "Usage: " << progName << " <command> [args]\n\n"
              << "Available commands:\n" << GREEN
              << "\trepo-add <name> <url>     Add a new repository\n"
              << "\trepo-remove <name>        Remove a repository\n"
              << "\trepo-enable <name>        Enable a repository\n"
              << "\trepo-disable <name>       Disable a repository\n"
              << "\trepo-list                 List all repositories\n"
              << "\tfetch                     Fetch updates from repositories\n"
              << "\tinstall <package>         Install a package\n"
              << "\tremove <package>          Remove a package\n"
              << "\tupdate                    Update installed packages\n"
              << "\tlock <package>            Lock a package\n"
              << "\tunlock <package>          Unlock a package\n"
              << "\tcleanup                   Clean up unused packages\n"
              << "\tkeep <package>            Keep a package during cleanup\n"
              << "\tunkeep <package>          Do not keep a package during cleanup\n"
              << "\tsearch <query>            Search for packages\n"
              << "\tinfo <package>            Show package information\n"
              << "\thelp                      Show this help message\n"
              << RESET;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << RED << "Error: No command specified\n" << RESET;
    printHelp(argv[0]);
    return 1;
  }

  fs::path homeDir = getenv("HOME");
  atlas::Atlas pm(homeDir / ".local/share/atlas", homeDir / ".cache/atlas",
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
    return pm.AddRepository(argv[2], argv[3]) ? 0 : 1;
  }

  if (command == "repo-remove") {
    if (argc != 3) {
      std::cout << RED << "Error: repo-remove requires repository name\n"
                << RESET;
      return 1;
    }
    return pm.RemoveRepository(argv[2]) ? 0 : 1;
  }

  if (command == "repo-enable") {
    if (argc != 3) {
      std::cout << RED << "Error: repo-enable requires repository name\n"
                << RESET;
      return 1;
    }
    return pm.EnableRepository(argv[2]) ? 0 : 1;
  }

  if (command == "repo-disable") {
    if (argc != 3) {
      std::cout << RED << "Error: repo-disable requires repository name\n"
                << RESET;
      return 1;
    }
    return pm.DisableRepository(argv[2]) ? 0 : 1;
  }

  if (command == "repo-list") {
    pm.ListRepositories();
    return 0;
  }

  if (command == "fetch") {
    return pm.Fetch() ? 0 : 1;
  }

  if (command == "install") {
    if (argc != 3) {
      std::cout << RED << "Error: install requires package name\n" << RESET;
      return 1;
    }
    return pm.Install(argv[2]) ? 0 : 1;
  }

  if (command == "remove") {
    if (argc != 3) {
      std::cout << RED << "Error: remove requires package name\n" << RESET;
      return 1;
    }
    return pm.Remove(argv[2]) ? 0 : 1;
  }

  if (command == "update") {
    return pm.Update() ? 0 : 1;
  }

  if (command == "lock") {
    if (argc != 3) {
      std::cout << RED << "Error: lock requires package name\n" << RESET;
      return 1;
    }
    return pm.LockPackage(argv[2]) ? 0 : 1;
  }

  if (command == "unlock") {
    if (argc != 3) {
      std::cout << RED << "Error: unlock requires package name\n" << RESET;
      return 1;
    }
    return pm.UnlockPackage(argv[2]) ? 0 : 1;
  }

  if (command == "cleanup") {
    if (argc != 2) {
      std::cout << RED << "Error: remove cleaning up packages\n" << RESET;
      return 1;
    }
    pm.Cleanup();
    return 0;
  }

  if (command == "keep") {
    if (argc != 3) {
      std::cout << RED << "Error: keep requires package name\n" << RESET;
      return 1;
    }
    return pm.KeepPackage(argv[2]) ? 0 : 1;
  }

  if (command == "unkeep") {
    if (argc != 3) {
      std::cout << RED << "Error: unkeep requires package name\n" << RESET;
      return 1;
    }
    return pm.UnkeepPackage(argv[2]) ? 0 : 1;
  }

  if (command == "search") {
    if (argc != 3) {
      std::cout << RED << "Error: search requires a query argument\n" << RESET;
      return 1;
    }
    for (const auto &result : pm.Search(argv[2]))
      std::cout << result << "\n";
    return 0;
  }

  if (command == "info") {
    if (argc != 3) {
      std::cout << RED << "Error: info requires package name\n" << RESET;
      return 1;
    }
    pm.Info(argv[2]);
    return 0;
  }

  std::cout << RED << "Error: Invalid command '" << command << "'\n" << RESET;
  printHelp(argv[0]);
  return 1;
}
