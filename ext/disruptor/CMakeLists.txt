include_directories( "${PROJECT_SOURCE_DIR}/ext" )
add_library( disruptor claim_strategy.cpp wait_strategy.cpp sequence.cpp)
