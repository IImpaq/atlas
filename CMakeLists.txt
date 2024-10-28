cmake_minimum_required(VERSION 3.29)
project(atlas)

set(CMAKE_CXX_STANDARD 26)

find_package(CURL CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)

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

target_link_libraries(${PROJECT_NAME} PRIVATE
        CURL::libcurl
        JsonCpp::JsonCpp
)