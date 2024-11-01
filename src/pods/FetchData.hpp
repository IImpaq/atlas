/**
* @file FetchData.hpp
* @author Marcus Gugacs
* @date 01.11.24
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#ifndef ATLAS_FETCH_DATA_HPP
#define ATLAS_FETCH_DATA_HPP

#include <data/Map.hpp>

#include "pods/PackageConfig.hpp"

namespace atlas {
  struct FetchData {
    ntl::Array<ntl::String> successful_fetchs;
    ntl::Array<ntl::String> skipped_fetchs;
    ntl::Array<ntl::String> failed_fetchs;
  };
}

#endif // ATLAS_FETCH_DATA_HPP
