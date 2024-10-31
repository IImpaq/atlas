//
// Created by Marcus Gugacs on 28.10.24.
//

#ifndef ATLAS_PACKAGE_CONFIG_HPP
#define ATLAS_PACKAGE_CONFIG_HPP

#include <vector>

#include <data/String.hpp>

struct PackageConfig {
    ntl::String name;
    ntl::String version;
    ntl::String description;
    ntl::String build_command;
    ntl::String install_command;
    ntl::String uninstall_command;
    ntl::String repository;
    std::vector<ntl::String> dependencies;
};

#endif // ATLAS_PACKAGE_CONFIG_HPP
