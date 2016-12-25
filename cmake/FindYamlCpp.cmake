# Locate yaml-cpp
#
# This module defines
#  YAMLCPP_FOUND, if false, do not try to link to yaml-cpp
#  YAMLCPP_LIBRARY, where to find yaml-cpp library
#  YAMLCPP_INCLUDE_DIR, where to find yaml.h

find_path(YAMLCPP_INCLUDE_DIR yaml-cpp/yaml.h
    PATHS ${CMAKE_BINARY_DIR}/ext/yaml-cpp/include)

find_library(YAMLCPP_LIBRARY NAMES yaml-cpp 
    PATHS ${CMAKE_BINARY_DIR}/ext/yaml-cpp/lib)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(YAMLCPP DEFAULT_MSG YAMLCPP_INCLUDE_DIR YAMLCPP_LIBRARY)
mark_as_advanced(YAMLCPP_INCLUDE_DIR YAMLCPP_LIBRARY)
