cmake_minimum_required(VERSION 3.27)
project(ninedb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build)

find_package(Boost REQUIRED)
find_package(lz4 REQUIRED)

add_executable(${PROJECT_NAME} test/kvdb.cpp)
target_link_libraries(${PROJECT_NAME} Boost::boost lz4::lz4)
