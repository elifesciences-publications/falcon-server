include_directories( "${PROJECT_SOURCE_DIR}/src/utilities" )
include_directories( "${PROJECT_SOURCE_DIR}/ext" )

#add_library( processors registerprocessors.cpp iconnection.cpp iport.cpp iprocessor.cpp dummysink.cpp testsink.cpp testsource.cpp testfilter.cpp testsinkmulti.cpp testsourcemulti.cpp nlxreader.cpp )
add_library( processors registerprocessors.cpp iconnection.cpp iport.cpp iprocessor.cpp dummysink.cpp nlxreader.cpp multichannelfilter.cpp openephysreader.cpp serialoutput.cpp )
