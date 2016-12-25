#include "registerprocessors.hpp"

#include "dummysink.hpp"
#include "nlxreader.hpp"
#include "multichannelfilter.hpp"
#include "eventsource.hpp"
#include "eventsink.hpp"
#include "eventsync.hpp"
#include "digitaloutput.hpp"
#include "levelcrossingdetector.hpp"
#include "rebuffer.hpp"
#include "fileserializer.hpp"
#include "zmqserializer.hpp"
#include "runningstats.hpp"
#include "spikedetector.hpp"
#include "multichanneldatafilestreamer.hpp"
#include "spikestreamer.hpp"
#include "muaestimator.hpp"
#include "burstdetector.hpp"

void registerProcessors() {
    
    REGISTERPROCESSOR(DummySink)
    REGISTERPROCESSOR(NlxReader)
    REGISTERPROCESSOR(MultiChannelFilter)
    REGISTERPROCESSOR(EventSource)
    REGISTERPROCESSOR(EventSink)
    REGISTERPROCESSOR(LevelCrossingDetector)
    REGISTERPROCESSOR(EventSync)
    REGISTERPROCESSOR(DigitalOutput)
    REGISTERPROCESSOR(Rebuffer)
    REGISTERPROCESSOR(FileSerializer)
    REGISTERPROCESSOR(ZMQSerializer)
    REGISTERPROCESSOR(RunningStats)
    REGISTERPROCESSOR(SpikeDetector)
    REGISTERPROCESSOR(MultichannelDataFileStreamer)
    REGISTERPROCESSOR(SpikeStreamer)
    REGISTERPROCESSOR(MUAEstimator)
    REGISTERPROCESSOR(BurstDetector)
}

