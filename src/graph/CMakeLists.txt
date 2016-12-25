include_directories( "${PROJECT_SOURCE_DIR}/src/utilities" )
include_directories( "${PROJECT_SOURCE_DIR}/src/processors" )
add_library( graph processorgraph.cpp graphmanager.cpp connectionparser.cpp connections.cpp processorengine.cpp iprocessor.cpp streamports.cpp threadutilities.cpp)
