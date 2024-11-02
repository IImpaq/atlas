/**
* @file PackageConfig.hpp
* @author Marcus Gugacs
* @date 28.10.24
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#ifndef ATLAS_PACKAGE_CONFIG_HPP
#define ATLAS_PACKAGE_CONFIG_HPP

#include <data/Array.hpp>
#include <data/String.hpp>

namespace atlas {
  /**
   * @struct PackageConfig
   * @brief Configuration data for a package in the Atlas framework.
   *
   * This struct holds metadata about a package, such as its name, version,
   * dependencies, and other relevant details. It is intended to be used by
   * the Atlas package manager to manage and install packages.
   */
  struct PackageConfig {
    ntl::String name;
    ntl::String version;
    ntl::String description;
    ntl::String build_command;
    ntl::String install_command;
    ntl::String uninstall_command;
    ntl::String repository;
    ntl::Array<ntl::String> dependencies;
  };
}

#endif // ATLAS_PACKAGE_CONFIG_HPP
