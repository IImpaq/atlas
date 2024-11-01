/**
* @file InstallerData.hpp
* @author Marcus Gugacs
* @date 01.11.24
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#ifndef INSTALLER_DATA_HPP
#define INSTALLER_DATA_HPP

#include <data/Map.hpp>

#include "pods/PackageConfig.hpp"

namespace atlas {
  struct InstallerData {
    ntl::Array<PackageConfig> configs;
    ntl::Array<ntl::String> successful_installs;
    ntl::Array<ntl::String> skipped_installs;
    ntl::Array<ntl::String> failed_installs;
    ntl::Map<ntl::String, bool> scheduled;
  };
}

#endif // INSTALLER_DATA_HPP
