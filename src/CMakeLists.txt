cmake_minimum_required (VERSION 2.8)
ENABLE_LANGUAGE(CXX)

project (falcon)

set(PLATFORM_LINK_LIBRARIES rt dl)

add_definitions(-DG2_DYNAMIC_LOGGING)

# list of source files

SET( sources   
    "main.cpp"
    "configuration/configuration.cpp"
    "commands/commandhandler.cpp"
    "commands/commandsource.cpp"
    "data/idata.cpp"
    "data/serialize.cpp"
    "data/eventdata.cpp"
    "data/spikedata.cpp"
    "data/muadata.cpp"
    "graph/graphmanager.cpp"
    "graph/processorgraph.cpp"
    "graph/connectionparser.cpp"
    "graph/istreamports.cpp"
    "graph/streamports.cpp"
    "graph/iprocessor.cpp"
    "graph/processorengine.cpp"
    "graph/threadutilities.cpp"
    "graph/connections.cpp"
)   

LIST(APPEND sources
    "processors/registerprocessors.cpp"
    "processors/nlxreader.cpp"
    "processors/dummysink.cpp"
    "processors/multichannelfilter.cpp"
    "processors/eventsource.cpp"
    "processors/eventsink.cpp"
    "processors/levelcrossingdetector.cpp"
    "processors/eventsync.cpp"
    "processors/digitaloutput.cpp"
    "processors/rebuffer.cpp"
    "processors/fileserializer.cpp"
    "processors/zmqserializer.cpp"
    "processors/runningstats.cpp"
    "processors/spikedetector.cpp"
    "processors/multichanneldatafilestreamer.cpp"
    "processors/spikestreamer"
    "processors/muaestimator.cpp"
    "processors/burstdetector.cpp"
    "processors/openephysreader.cpp"
    "processors/serialoutput.cpp"
    "processors/rippledetector.cpp"
)

include_directories( "../ext" )
include_directories( "../lib" )

add_executable(falcon ${sources})
target_link_libraries (falcon logging loggers disruptor zmq utilities dsp dio neuralynx npyreader openephys arduino-serial ${YAMLCPP_LIBRARY} ${PLATFORM_LINK_LIBRARIES})
