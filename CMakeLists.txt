cmake_minimum_required(VERSION 3.1)
project(ninedb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/conan)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/conan)

find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(lz4 REQUIRED)

add_executable(${PROJECT_NAME} test/kvdb.cpp)
target_link_libraries(${PROJECT_NAME} Boost::boost lz4::lz4 nlohmann_json::nlohmann_json)
