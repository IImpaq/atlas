//
// Created by Marcus Gugacs on 01.11.24.
//

#ifndef FETCH_DATA_HPP
#define FETCH_DATA_HPP

#include <data/Map.hpp>

#include "pods/PackageConfig.hpp"

namespace atlas {
  struct FetchData {
    ntl::Array<ntl::String> successful_fetchs;
    ntl::Array<ntl::String> skipped_fetchs;
    ntl::Array<ntl::String> failed_fetchs;
  };
}

#endif // FETCH_DATA_HPP
