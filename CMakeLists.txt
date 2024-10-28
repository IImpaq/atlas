cmake_minimum_required(VERSION 3.29)
project(atlas)

set(CMAKE_CXX_STANDARD 26)

find_package(CURL CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)

add_executable(atlas main.cpp)

target_link_libraries(atlas PRIVATE
        CURL::libcurl
        JsonCpp::JsonCpp
)
