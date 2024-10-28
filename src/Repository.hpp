//
// Created by Marcus Gugacs on 28.10.24.
//

#ifndef ATLAS_REPOSITORY_HPP
#define ATLAS_REPOSITORY_HPP

#include <string>

struct Repository {
    std::string name;
    std::string url;
    std::string branch;
    bool enabled;
};

#endif // ATLAS_REPOSITORY_HPP
