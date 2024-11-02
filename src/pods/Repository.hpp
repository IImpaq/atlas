/**
* @file Repository.hpp
* @author Marcus Gugacs
* @date 28.10.24
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#ifndef ATLAS_REPOSITORY_HPP
#define ATLAS_REPOSITORY_HPP

#include <data/Bool.hpp>
#include <data/String.hpp>

namespace atlas {
  /**
   * @struct Repository
   *
   * @brief This struct represents a repository, which can be thought of as a collection or container of data.
   *
   * A repository is typically used to manage and organize data in a way that's accessible and useful for the application.
   */
  struct Repository {
    ntl::String name;
    ntl::String url;
    ntl::String branch;
    bool enabled;
  };
}

#endif // ATLAS_REPOSITORY_HPP
