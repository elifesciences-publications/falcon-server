add_library( dsp filter.cpp algorithms.cpp behavior_algorithms.cpp)
target_link_libraries( dsp ${YAMLCPP_LIBRARY} )
