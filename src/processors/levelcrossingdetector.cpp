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

#include "levelcrossingdetector.hpp"
#include "g3log/src/g2log.hpp"

void LevelCrossingDetector::Configure( const YAML::Node & node, const GlobalContext& context ) {
    
    initial_threshold_ = node["threshold"].as<decltype(initial_threshold_)>(
        DEFAULT_THRESHOLD);
    initial_upslope_ = node["upslope"].as<decltype(initial_upslope_)>(
        DEFAULT_UPSLOPE);
    initial_post_detect_block_ = node["post_detect_block"].as<decltype(
        initial_post_detect_block_)>(DEFAULT_POST_DETECT_BLOCK);
    
    std::string event_name = node["event"].as<std::string>(DEFAULT_EVENT);
    event_prototype_ = EventData( event_name );
    
}

void LevelCrossingDetector::CreatePorts( ) {
    
    data_in_port_ = create_input_port(
        "data",
        MultiChannelDataType<double>( ChannelRange(1,256) ),
        PortInPolicy( SlotRange(1) ) );
    
    data_out_port_ = create_output_port(
        "events",
        EventDataType(),
        PortOutPolicy( SlotRange(1) ) );
    
    threshold_ = create_readable_shared_state(
        "threshold", initial_threshold_,
        Permission::READ,
        Permission::WRITE);
    
    upslope_ = create_readable_shared_state(
        "upslope",
        initial_upslope_,
        Permission::READ,
        Permission::WRITE);
    
    post_detect_block_ = create_readable_shared_state(
        "post_detect_block",
        initial_post_detect_block_,
        Permission::READ,
        Permission::WRITE);
}

void LevelCrossingDetector::Preprocess( ProcessingContext& context ) {
    
    double init_value;
    
    post_detection_block_update( initial_post_detect_block_ );
    
    if (upslope_->get()) {
        init_value = std::numeric_limits<int>::max();
    } else {
        init_value = std::numeric_limits<int>::min();
    }
    
    previous_sample_.assign( data_in_port_->streaminfo(0).datatype().nchannels(), init_value );
}

void LevelCrossingDetector::Process( ProcessingContext& context ) {
    
    double threshold = 0;
    bool upslope = false;
    unsigned int post_detect_block = initial_post_detect_block_;
    unsigned int post_detect_block_old = initial_post_detect_block_;
    bool crossing_detected = false;
    unsigned int nblock = 0;
    
    while (!context.terminated()) {
        
        if (!data_in_port_->slot(0)->RetrieveData(data_in_)) {break;}
        
        threshold = threshold_->get();
        upslope = upslope_->get();
        post_detect_block_old = post_detect_block; post_detect_block = post_detect_block_->get();
        
        if (post_detect_block_old != post_detect_block) {
            post_detection_block_update( post_detect_block );
        }
        
        // if blocking and post_detect_block value changed to a lower value, make
        // sure to update the current block value
        if (nblock > post_detect_block) { nblock = post_detect_block; }
        
        // loop through each sample
        for ( unsigned int s=0; s<data_in_->nsamples(); ++s ) {
            
            if (nblock>0) {
                
                --nblock;
                
                if (nblock==0) {
                    for (unsigned int c=0; c<data_in_->nchannels(); ++c ) {
                        previous_sample_[c] = data_in_->data_sample(s,c);
                    }
                }
                
                continue;
            }
            
            // loop through each channel
            for (unsigned int c=0; c<data_in_->nchannels(); ++c ) {
                       
                // for up slope:
                if ( (upslope && (previous_sample_[c] <= threshold) && (data_in_->data_sample(s,c) > threshold)) ||
                     (!upslope && (previous_sample_[c] >= threshold) && (data_in_->data_sample(s,c) < threshold)) ) {
                    
                    crossing_detected = true;
                    break;
                }
            }
            
            if (crossing_detected) {
                data_out_ = data_out_port_->slot(0)->ClaimData(false);
                
                data_out_->set_source_timestamp( data_in_->source_timestamp() );
                data_out_->set_hardware_timestamp( data_in_->sample_timestamp( s ) );
                data_out_->set_serial_number( data_in_->serial_number() );
                
                data_out_->set_event( event_prototype_ );
                data_out_port_->slot(0)->PublishData();
                
                crossing_detected = false;
                ++ n_detections_;
                
                nblock = post_detect_block;
                
                if ( (n_detections_ % 50) == 0 ) {
                    LOG(DEBUG) << name() << ". " << n_detections_ << " detections of event "
                    << event_prototype_.event() << " occurred.";

                }
            }  
            
            for (unsigned int c=0; c<data_in_->nchannels(); ++c ) {
                previous_sample_[c] = data_in_->data_sample(s,c);
            }
            
        }
        
        data_in_port_->slot(0)->ReleaseData();
    }
}


void LevelCrossingDetector::Postprocess( ProcessingContext& context ) {
    
    LOG(INFO) << name() << ". " << n_detections_ << " detections of event "
        << event_prototype_.event() << " occurred.";
    n_detections_ = 0;
}

void LevelCrossingDetector::post_detection_block_update(
decltype(initial_post_detect_block_) post_detection_block ) {
        
    double post_detection_block_us =
        post_detection_block / data_in_port_->streaminfo(0).datatype().sample_rate() * 1e6;
    
    LOG(INFO) << name() << ". Post-detection block is set to " << post_detection_block_us << " microseconds.";
    
    if (post_detection_block_us < LOW_POST_DETECTION_BLOCK_US) {
        LOG(WARNING) << name() << ". Post-detection block might be too low!"; 
    }
}
