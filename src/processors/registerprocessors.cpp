// ---------------------------------------------------------------------
// This file is part of falcon-server.
// 
// Copyright (C) 2015, 2016, 2017 Neuro-Electronics Research Flanders
// 
// Falcon-server is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Falcon-server is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with falcon-server. If not, see <http://www.gnu.org/licenses/>.
// ---------------------------------------------------------------------

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
#include "rippledetector.hpp"
#include "likelihoodsync.hpp"
#include "behaviorestimator.hpp"
#include "spikedetector.hpp"
#include "decoder.hpp"
#include "replayidentifier.hpp"
#include "multichanneldatafilestreamer.hpp"
#include "spikestreamer.hpp"
#include "likelihooddatafilestreamer.hpp"
#include "videotrackreader.hpp"
#include "videotrackdatafilestreamer.hpp"
#include "eventfilter.hpp"
#include "behaviorreader.hpp"
#include "muaestimator.hpp"
#include "burstdetector.hpp"
#include "serialoutput.hpp"
#include "openephysreader.hpp"
#include "nlxpurereader.hpp"
#include "dispatcher.hpp"
#include "nlxparser.hpp"
#include "eventconverter.hpp"

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
    REGISTERPROCESSOR(RippleDetector)
    REGISTERPROCESSOR(LikelihoodSync)
    REGISTERPROCESSOR(BehaviorEstimator)
    REGISTERPROCESSOR(SpikeDetector)
    REGISTERPROCESSOR(Decoder)
    REGISTERPROCESSOR(ReplayIdentifier)
    REGISTERPROCESSOR(MultichannelDataFileStreamer)
    REGISTERPROCESSOR(SpikeStreamer)
    REGISTERPROCESSOR(LikelihoodDataFileStreamer)
    REGISTERPROCESSOR(VideoTrackReader)
    REGISTERPROCESSOR(VideoTrackDataFileStreamer)
    REGISTERPROCESSOR(EventFilter)
    REGISTERPROCESSOR(BehaviorReader)
    REGISTERPROCESSOR(MUAEstimator)
    REGISTERPROCESSOR(BurstDetector)
    REGISTERPROCESSOR(SerialOutput)
    REGISTERPROCESSOR(OpenEphysReader)
    REGISTERPROCESSOR(NlxPureReader)
    REGISTERPROCESSOR(Dispatcher)
    REGISTERPROCESSOR(NlxParser)
    REGISTERPROCESSOR(EventConverter)
}

