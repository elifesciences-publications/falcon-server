project( test_filter )

set(CMAKE_CXX_FLAGS "-O3 -Wall -Wunused -std=c++11")

include_directories( "../lib" )
include_directories( "../ext" )

add_executable( test_filter test_filter.cpp )
target_link_libraries (test_filter utilities dsp)
