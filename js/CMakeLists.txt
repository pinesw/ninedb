cmake_minimum_required(VERSION 3.27)
project(ninedb LANGUAGES CXX)

message(CONAN_PATH="${CONAN_PATH}")
message(NODE_API_HEADERS="${NODE_API_HEADERS}")

set(CMAKE_CXX_STANDARD 17)

add_definitions(-DNAPI_VERSION=3)
include_directories(${NODE_API_HEADERS}/include)

list(APPEND CMAKE_MODULE_PATH ${CONAN_PATH})
list(APPEND CMAKE_PREFIX_PATH ${CONAN_PATH})
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(lz4 REQUIRED)

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)

add_library(${PROJECT_NAME} SHARED ${CMAKE_CURRENT_SOURCE_DIR}/binding.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} Boost::boost lz4::lz4 nlohmann_json::nlohmann_json)

# if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
#     target_link_libraries(${PROJECT_NAME} -static-libgcc -static-libstdc++)
# endif()

if(MSVC)
    SET(CMAKE_CXX_FLAGS "/EHsc")
    execute_process(COMMAND ${CMAKE_AR} /def:${NODE_API_HEADERS}/def/node_api.def /out:${CMAKE_BINARY_DIR}/node_api.lib ${CMAKE_STATIC_LINKER_FLAGS})
    target_link_libraries(${PROJECT_NAME} node_api)
endif(MSVC)

# get_property(vars DIRECTORY PROPERTY VARIABLES)
# foreach (var IN LISTS vars)
#   message(STATUS "${var} = ${${var}}")
# endforeach ()
