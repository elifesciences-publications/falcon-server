cmake_minimum_required (VERSION 2.8)
ENABLE_LANGUAGE(CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -rdynamic -Wunused -std=c++11 -pthread -D_GLIBCXX_USE_NANOSLEEP")

project (rootproject)

# installing third party libraries dependencies
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/BuildExternalProject.cmake)
build_external_project(yaml-cpp "${CMAKE_SOURCE_DIR}/ext/yaml-cpp-0.5.3" "-DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/ext/yaml-cpp -DYAML_CPP_BUILD_TOOLS=OFF -DYAML_CPP_BUILD_CONTRIB=OFF")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(YamlCpp REQUIRED)

include_directories(SYSTEM "${YAMLCPP_INCLUDE_DIR}")

add_subdirectory( ext )
add_subdirectory( lib )
add_subdirectory( src )
add_subdirectory( tools )
add_subdirectory( tests )

