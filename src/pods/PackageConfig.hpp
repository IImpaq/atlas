//
// Created by Marcus Gugacs on 28.10.24.
//

#ifndef ATLAS_PACKAGE_CONFIG_HPP
#define ATLAS_PACKAGE_CONFIG_HPP

#include <string>
#include <vector>

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

#endif // ATLAS_PACKAGE_CONFIG_HPP
