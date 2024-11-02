cmake_minimum_required(VERSION 3.26)
project(atlas)

set(CMAKE_CXX_STANDARD 23)

message(" - Setting up make directories...")
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/)

message(" - Scanning for source files...")
file(GLOB_RECURSE SOURCES
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/src/*.hpp)

include_directories(${CMAKE_SOURCE_DIR}/src/)

message(" - Creating executable...")
add_executable(${PROJECT_NAME} ${SOURCES})

message(" - Configuring third party packages...")
find_package(CURL CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)

find_package(Z3 CONFIG REQUIRED)

pkg_check_modules(tomlplusplus REQUIRED IMPORTED_TARGET tomlplusplus)

add_subdirectory(libs/NTL/ntl/)
target_include_directories(${PROJECT_NAME} PUBLIC libs/NTL/ntl/src)

target_link_libraries(${PROJECT_NAME} PRIVATE
        CURL::libcurl
        JsonCpp::JsonCpp
        PkgConfig::tomlplusplus
        z3::libz3
        ntl
)