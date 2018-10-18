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

#include "likelihooddatafilestreamer.hpp"

#include "g3log/src/g2log.hpp"
#include "utilities/math_numeric.hpp"
#include "utilities/string.hpp"

constexpr double LikelihoodDataFileStreamer::DEFAULT_TIMEBIN_MS;
constexpr double LikelihoodDataFileStreamer::DEFAULT_SAMPLE_RATE;
constexpr double LikelihoodDataFileStreamer::DEFAULT_STREAMING_RATE;

void LikelihoodDataFileStreamer::Configure(const YAML::Node& node, const GlobalContext& context) {
    
    path_to_likelihood_ =
        context.resolve_path( node["path_to_likelihood"].as<std::string>( ) );
    path_to_n_spikes_ =
        context.resolve_path( node["path_to_n_spikes"].as<std::string>( ) );
    time_bin_ms_ = node["time_bin_ms"].as<double>( );
    sample_rate_ = node["sample_rate"].as<double>( DEFAULT_SAMPLE_RATE );
    streaming_rate_ = node["streaming_rate"].as<double>( DEFAULT_STREAMING_RATE );
    initial_timestamp_ = node["initial_timestamp"].as<double>( DEFAULT_INITIAL_TS );
    
    // check configuration parameters
    if (!path_exists(path_to_likelihood_)) {
        throw ProcessingConfigureError(
            "This file path to likelihood doesn't exit: " + path_to_likelihood_,
            name());
    }
    if (!path_exists(path_to_n_spikes_)) {
        throw ProcessingConfigureError(
            "This file path to n_spikes doesn't exit: " + path_to_n_spikes_,
            name());
    }
    if ( time_bin_ms_ < 0 ) {
        throw ProcessingConfigureError("Time must be a positive number", name());
    }
    if ( sample_rate_ <= 0 ) {
        throw ProcessingConfigureError("Sample rate must be a positive number.",
            name());
    }  
    if ( streaming_rate_ <= 0 ) {
        throw ProcessingConfigureError("Streaming rate must be a positive number.",
            name());
    }
    
    // open the likelihood NPY file and 
    // extract the grid size and the number of items that have to be streamed
    if ( (fp_likelihood_ = fopen(path_to_likelihood_.c_str(), "r")) == NULL ) {
        throw ProcessingConfigureError(
            "Cannot open the NPY file. Check the file path " + path_to_likelihood_,
            name());
    }
    
    if ( get_2D_matrix_shape(fp_likelihood_, &n_packets_to_stream_, &grid_size_) != 0 ) {
        throw ProcessingPrepareError("Cannot read the shape of the NPY file.",
            name());
    } else {
        LOG(INFO) << name() << ". Loaded " << n_packets_to_stream_ <<
            " likelihoods having a grid size of " << grid_size_ << ".";
    }
}

void LikelihoodDataFileStreamer::CreatePorts() {
    
    data_out_port_ = create_output_port(
        "estimates",
        LikelihoodDataType(),
        PortOutPolicy( SlotRange(1) ) );
}

void LikelihoodDataFileStreamer::CompleteStreamInfo( ) {
    
    data_out_port_->streaminfo(0).datatype().Finalize( time_bin_ms_, grid_size_ );
    data_out_port_->streaminfo(0).Finalize( streaming_rate_ );
}

void LikelihoodDataFileStreamer::Prepare( GlobalContext& context) {
    
    // open the n_spikes NPY file
    if ( (fp_n_spikes_ = fopen(path_to_n_spikes_.c_str(), "r")) == NULL ) {
        throw ProcessingPrepareError(
            "Cannot open the NPY file. Check the file path " + path_to_likelihood_,
            name());
    }
    
    // check the consistency of the number of items that have to be streamed
    uint32_t n_bins = 0;
    if ( get_1D_array_len( fp_n_spikes_, &n_bins ) != 0 ) {
        throw ProcessingPrepareError(
            "Cannot read the length of the NPY file", name());
    } else if (n_bins != n_packets_to_stream_) {
        throw ProcessingPrepareError(
            "Number of likelihood bins is inconsistent between in the two files",
            name());
    }
    
    // load the data
    if ( (loaded_log_likelihoods_ = get_2D_matrix_f64( fp_likelihood_ )) == NULL ) {
        throw ProcessingPrepareError(
            "Cannot load the likelihood data from the NPY file", name());
    }
    if ( (loaded_n_spikes_ = retrieve_npy_int32( fp_n_spikes_ )) == NULL ) {
        throw ProcessingPrepareError(
            "Cannot load the number of spikes from the NPY file", name());
    }
    LOG(INFO) << name() << ". Data was loaded correctly from the two files.";
    
    // generate hardware timestamps
    double last_time_us =   initial_timestamp_ + 
                            (n_packets_to_stream_ - 1) * time_bin_ms_ * 1e3;
    
    std::vector<double> hw_timestamps =
        linspace(   static_cast<double>(initial_timestamp_), last_time_us,
                    n_packets_to_stream_);
    for (auto& el: hw_timestamps) {
        generated_hw_timestamps_.push_back( static_cast<uint64_t>( std::round(el) ) );
    }
    LOG(DEBUG) << name() << ". First generated TS: " << generated_hw_timestamps_[0]
        << ". Last generated TS: " << generated_hw_timestamps_[n_packets_to_stream_-1];
    LOG(DEBUG) << name() << ". First n_spike: " << loaded_n_spikes_[0]
        << ". Last n_spike: " << loaded_n_spikes_[n_packets_to_stream_-1];
}

void LikelihoodDataFileStreamer::Process(ProcessingContext& context) {
    
    LikelihoodData* data = nullptr;
    auto delay = std::chrono::microseconds( static_cast<unsigned int>(
        1e6 / streaming_rate_ ) );  
    decltype(n_packets_to_stream_) i = 0;
    
    while ( !context.terminated() &&
            data_out_port_->slot(0)->nitems_produced() < n_packets_to_stream_ ) {
        
        data = data_out_port_->slot(0)->ClaimData( true );
        
        data->set_time_bin( time_bin_ms_ );
        data->add_spikes( loaded_n_spikes_[i] );
        data->set_log_likelihood( loaded_log_likelihoods_[i], grid_size_ );
        data->set_hardware_timestamp( generated_hw_timestamps_[i] );
        data->set_source_timestamp();
        
        ++ i;
        
        data_out_port_->slot(0)->PublishData();
        
        std::this_thread::sleep_for( delay );
    }   
}

void LikelihoodDataFileStreamer::Postprocess( ProcessingContext& context ) {
    
    LOG(INFO) << name()<< ". STREAMED " << data_out_port_->slot(0)->nitems_produced()
        << " data packets. PRESS 's' to STOP THE GRAPH";
}

void LikelihoodDataFileStreamer::Unprepare( GlobalContext& context) {

    fclose(fp_likelihood_); fp_likelihood_ = nullptr;
    fclose(fp_n_spikes_); fp_n_spikes_ = nullptr;
    for (decltype(n_packets_to_stream_) i=0; i < n_packets_to_stream_; ++i) {
        free(loaded_log_likelihoods_[i]); loaded_log_likelihoods_[i] = nullptr;
    }
    free(loaded_n_spikes_); loaded_n_spikes_ = nullptr;
    free(loaded_log_likelihoods_); loaded_log_likelihoods_ = nullptr;
}

