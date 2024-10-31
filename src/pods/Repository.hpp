//
// Created by Marcus Gugacs on 28.10.24.
//

#ifndef ATLAS_REPOSITORY_HPP
#define ATLAS_REPOSITORY_HPP

#include <data/Bool.hpp>
#include <data/String.hpp>

struct Repository {
    ntl::String name;
    ntl::String url;
    ntl::String branch;
    bool enabled;
};

#endif // ATLAS_REPOSITORY_HPP
