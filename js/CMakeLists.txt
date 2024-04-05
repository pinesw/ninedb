cmake_minimum_required(VERSION 3.27)
project(ninedb LANGUAGES CXX)

message(CONAN_PATH="${CONAN_PATH}")
message(NODE_API_HEADERS="${NODE_API_HEADERS}")

set(CMAKE_CXX_STANDARD 17)

add_definitions(-DNAPI_VERSION=3)
include_directories(${NODE_API_HEADERS}/include)
link_directories(${NODE_API_HEADERS}/lib)

list(APPEND CMAKE_MODULE_PATH ${CONAN_PATH})
list(APPEND CMAKE_PREFIX_PATH ${CONAN_PATH})
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(lz4 REQUIRED)

add_library(${PROJECT_NAME} SHARED ${CMAKE_CURRENT_SOURCE_DIR}/binding.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} -static-libgcc -static-libstdc++)
target_link_libraries(${PROJECT_NAME} Boost::boost lz4::lz4 nlohmann_json::nlohmann_json)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} node_api)
endif()
