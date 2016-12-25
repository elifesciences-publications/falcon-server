include_directories( "${PROJECT_SOURCE_DIR}/ext" )

# disruptor library
add_library( disruptor disruptor/claim_strategy.cpp disruptor/wait_strategy.cpp disruptor/sequence.cpp)

# g3log library
add_definitions(-DG2_DYNAMIC_LOGGING)

set(G3LOG_SRC ./g3log/src)

file(GLOB SRC_FILES ${G3LOG_SRC}/*.h ${G3LOG_SRC}/*.hpp ${G3LOG_SRC}/*.cpp ${G3LOG_SRC}/*.ipp)
file(GLOB EXCL_FILE ${G3LOG_SRC}/crashhandler_win.cpp)
list(REMOVE_ITEM SRC_FILES ${EXCL_FILE})

set(SRC_FILES ${SRC_FILES})

add_library( logging ${SRC_FILES})

