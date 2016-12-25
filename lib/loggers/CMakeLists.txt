
#set(LOG_SRC ../../ext/g3log/src)

## GENERIC STEPS
#file(GLOB SRC_FILES ${LOG_SRC}/*.h ${LOG_SRC}/*.hpp ${LOG_SRC}/*.cpp ${LOG_SRC}/*.ipp)
#file(GLOB EXCL_FILE ${LOG_SRC}/crashhandler_win.cpp)
#list(REMOVE_ITEM SRC_FILES  ${EXCL_FILE})

#set(SRC_FILES ${SRC_FILES} ${SRC_PLATFORM_SPECIFIC})

## Create the g2log library
##include_directories(${LOG_SRC})
#add_library( logging ${SRC_FILES} customsink.cpp)
#set_target_properties( logging PROPERTIES LINKER_LANGUAGE CXX)

include_directories( "../../ext" )
add_library( loggers customsink.cpp )
