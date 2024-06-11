# Set up the project
cmake_minimum_required(VERSION 3.27)
project(ninedb LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# Print arguments for debugging
message(CONAN_PATH="${CONAN_PATH}")
message(JDK_HOME="${JDK_HOME}")

# Set output directories to be consistent across all platforms
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR})
endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)

# Configure Java API
if(WIN32)
    include_directories(${JDK_HOME}/include ${JDK_HOME}/include/win32)
elseif(APPLE)
    include_directories(${JDK_HOME}/include ${JDK_HOME}/include/darwin)
elseif(UNIX)
    include_directories(${JDK_HOME}/include ${JDK_HOME}/include/linux)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# Find dependencies via Conan
list(APPEND CMAKE_MODULE_PATH ${CONAN_PATH})
list(APPEND CMAKE_PREFIX_PATH ${CONAN_PATH})
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
find_package(Boost REQUIRED)
find_package(lz4 REQUIRED)

# Set up the target
add_library(${PROJECT_NAME} SHARED ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/io_pinesw_ninedb.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(${PROJECT_NAME} Boost::boost lz4::lz4)

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".dll")
elseif(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "lib" SUFFIX ".dylib")
elseif(UNIX)
    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "lib" SUFFIX ".so")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()
