include_directories( "../../lib" )
add_library(environment environment.cpp)
target_link_libraries( environment npyreader gmm )