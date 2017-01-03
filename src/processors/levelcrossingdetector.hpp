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

/* LevelCrossingDetector: detects a threshold crossing on any of the
 * channels in the incoming MultiChannelData stream and emits an event
 * in response
 * 
 * input ports:
 * data <MultiChannelData> (1 slot)
 *
 * output ports:
 * events <EventData> (1 slot)
 *
 * exposed states:
 * threshold <double> - threshold that needs to be crossed
 * upslope <bool> - whether to look for upward (true) or downward (false)
 *   threshold crossings
 * post_detect_block <unsigned int> - refractory period after threshold 
 *   crossing detection (in number of samples )
 *
 * exposed methods:
 * none
 *
 * options:
 * threshold <double> - default threshold state
 * upslope <bool> - default upslope state
 * post_detect_block <unsigned int> - default post_detect_block state
 * event <string> - event to emit upon detection of threshold crossing
 * 
 */

#ifndef LEVELCROSSINGDETECTOR_HPP
#define LEVELCROSSINGDETECTOR_HPP

#include "../graph/iprocessor.hpp"
#include "../data/eventdata.hpp"
#include "../data/multichanneldata.hpp"


class LevelCrossingDetector : public IProcessor {

public:
    virtual void Configure( const YAML::Node  & node, const GlobalContext& context) override;
    virtual void CreatePorts( ) override;
    virtual void Preprocess( ProcessingContext& context) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;
    
protected:
    PortIn<MultiChannelDataType<double>>* data_in_port_;
    PortOut<EventDataType>* data_out_port_;
    
    ReadableState<double>* threshold_;
    ReadableState<bool>* upslope_;
    ReadableState<unsigned int>* post_detect_block_;
    
    double initial_threshold_;
    bool initial_upslope_;
    unsigned int initial_post_detect_block_;
    
    std::vector<double> previous_sample_;
    
    EventData event_prototype_;
    uint64_t n_detections_;
    
    MultiChannelData<double>* data_in_;
    EventData* data_out_;
    
protected:
    void post_detection_block_update(
        decltype(initial_post_detect_block_) post_detection_block );

public:
    const decltype(initial_threshold_) DEFAULT_THRESHOLD = 0.0;
    const decltype(initial_upslope_) DEFAULT_UPSLOPE = true;
    const decltype(initial_post_detect_block_) DEFAULT_POST_DETECT_BLOCK = 2;
    const std::string DEFAULT_EVENT = "threshold_crossing";  
    const decltype(initial_post_detect_block_) LOW_POST_DETECTION_BLOCK_US = 30;
    
};

#endif // levelcrossingdetector.hpp
