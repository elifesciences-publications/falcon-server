include_directories( "../../lib" )
include_directories( "../../ext" )
add_library( encoding encodingmodel.cpp )
target_link_libraries( encoding gmm cnpy )
