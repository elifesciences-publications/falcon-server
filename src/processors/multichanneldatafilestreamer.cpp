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

#include "multichanneldatafilestreamer.hpp"
#include "npyreader/npyreader.h"
#include "g3log/src/g2log.hpp"
#include <chrono>
#include <thread>

constexpr unsigned int MultichannelDataFileStreamer::DEFAULT_BATCH_SIZE;
constexpr double MultichannelDataFileStreamer::DEFAULT_STREAMING_RATE;

void MultichannelDataFileStreamer::Configure(const YAML::Node& node, const GlobalContext& context) {
    
    filepath_ = context.resolve_path( node["filepath"].as<std::string>() );
    sample_rate_ = node["sample_rate"].as<double>( );
    batch_size_ = node["batch_size"].as<unsigned int>( DEFAULT_BATCH_SIZE );
    streaming_rate_ = node["streaming_rate"].as<double>( DEFAULT_STREAMING_RATE );
    initial_timestamp_ = node["initial_timestamp"].as<uint64_t>( DEFAULT_INITIAL_TIMESTAMP );
    
    // check configuration parameters
    if ( sample_rate_ <= 0 ) {
       throw ProcessingConfigureError("Sample rate must be a positive.", name());
    }  
    if ( batch_size_ == 0 ) {
        throw ProcessingConfigureError("Batch size cannot be zero", name());
    }
    if ( streaming_rate_ <= 0 ) {
        throw ProcessingConfigureError("Streaming rate must be a positive.", name());
    }
    
    // load shape from NPY file
    if ( (fp_ = fopen(filepath_.c_str(), "r")) == NULL ) {
        throw ProcessingConfigureError("Cannot open the NPY file. Check the file path " + filepath_, name());
    }
    if ( get_2D_matrix_shape(fp_, &n_channels_, &n_samples_) != 0 ) {
        throw ProcessingConfigureError("Cannot read the shape of the NPY file.", name());
    } else {
        LOG(INFO) << name() << ". Loaded " << n_channels_ << " channels with " << n_samples_
            << " samples for each channel.";
    }
}

void MultichannelDataFileStreamer::CreatePorts() {
    
    data_port_ = create_output_port(
        "data",
        MultiChannelDataType<double>( n_channels_ ),
        PortOutPolicy( SlotRange(1) ) );
}

void MultichannelDataFileStreamer::CompleteStreamInfo( ) {
    
    data_port_->streaminfo(0).datatype().Finalize(batch_size_, n_channels_, sample_rate_);
    data_port_->streaminfo(0).Finalize( sample_rate_ / batch_size_ );
}

void MultichannelDataFileStreamer::Prepare( GlobalContext& context ) {

    n_packets_to_dispatch_ = n_samples_ / batch_size_;
    LOG(INFO) << name() << ". " << n_packets_to_dispatch_ << " packets of MultiChannelData "
        << "will be streamed on the output port.";
    
    auto remaining_samples = n_samples_ % batch_size_;
    LOG_IF(INFO, (remaining_samples == 0)) << name() << ". All loaded samples will be streamed";
    LOG_IF(INFO, (remaining_samples != 0)) << name() << ". " << remaining_samples
        << " last samples will not be streamed (batch_size = " << batch_size_ << " ).";
    
    sampling_period_ = 1e6 / sample_rate_; // converted from Hz to microseconds
    
    // load data
    loaded_data_ = get_2D_matrix_f64( fp_ );
    if ( loaded_data_ == NULL ) {
        throw ProcessingPrepareError( "Couldn't load data correctly.", name());
    }
    
    // generate timestamps
    double data_period_us = static_cast<double>(n_samples_) / sample_rate_ * 1e6;
    std::vector<double> hw_timestamps =
        linspace(   static_cast<double>(initial_timestamp_),
                    initial_timestamp_ + data_period_us - sampling_period_,
                    n_samples_);
    for (auto& el: hw_timestamps) {
        generated_hw_timestamps_.push_back( static_cast<uint64_t>( std::round(el) ) );
    }
    LOG(DEBUG) << name() << ". First generated TS: " << generated_hw_timestamps_[0]
        << ". Last generated TS: " << generated_hw_timestamps_[n_samples_-1];
}

void MultichannelDataFileStreamer::Process(ProcessingContext& context) {
    
    MultiChannelData<double>* data = nullptr;
    auto delay = std::chrono::microseconds( static_cast<unsigned int>( 1e6 / streaming_rate_ ) );
    decltype(batch_size_) s = 0; // will loop through all loaded data and generated hw timestamps
    decltype(batch_size_) sample_index = 0, data_sample_idx = 0;;
    decltype(n_channels_) c = 0;;

    while ( !context.terminated() && data_port_->slot(0)->nitems_produced() < n_packets_to_dispatch_ ) {
        
        data = data_port_->slot(0)->ClaimData( false );
        
        data->set_source_timestamp();
        data->set_hardware_timestamp( generated_hw_timestamps_[s] ); 
        
        // loop through loaded data and copy to MultiChannelData item
        data_sample_idx = 0; // MCD index
        for ( s = sample_index; s < sample_index + batch_size_; ++ s ) { 
            for ( c = 0; c < n_channels_; ++c ) {
                data->set_data_sample( data_sample_idx, c, loaded_data_[c][s] );
            }
            data->set_sample_timestamp(data_sample_idx, generated_hw_timestamps_[s] );
            ++ data_sample_idx;
        }
        
        data_port_->slot(0)->PublishData();
        
        sample_index += batch_size_;
        
        std::this_thread::sleep_for( delay );
    }  
}

void MultichannelDataFileStreamer::Postprocess( ProcessingContext& context ) {
    
    LOG(INFO) << name()<< ". Streamed " << data_port_->slot(0)->nitems_produced()
        << " data packets. Graph can be stopped.";
}

void MultichannelDataFileStreamer::Unprepare( GlobalContext& context ) {

    fclose(fp_); fp_ = nullptr;
    for (decltype(n_channels_) i=0; i < n_channels_; ++i) {
        free(loaded_data_[i]); loaded_data_[i] = nullptr;
    }
    free(loaded_data_); loaded_data_ = nullptr;
}
