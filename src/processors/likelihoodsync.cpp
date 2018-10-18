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

#include "likelihoodsync.hpp"
#include "g3log/src/g2log.hpp"
#include "utilities/math_numeric.hpp"

void LikelihoodSync::Configure(const YAML::Node& node, const GlobalContext& context) {
    
    time_bin_ms_ = node["time_bin_ms"].as<double>( NO_BIN_SIZE ); 
    
    if ((time_bin_ms_ != NO_BIN_SIZE) and (time_bin_ms_ < 0)) {
        // tiny bug here in case user insert the value of NO_BIN_SIZE
        LOG(ERROR) << name() << ". You have inserted an invalid time bin: " <<
                time_bin_ms_;
        throw ProcessingConfigureError("Time bin must be a positive number.", name());
    }
}

void LikelihoodSync::CreatePorts() {
    
    data_in_port_ = create_input_port(
        "estimates",
        LikelihoodDataType(),
        PortInPolicy( SlotRange(1, 64) ) );
    
    data_out_port_ = create_output_port(
        "estimates",
        LikelihoodDataType(),
        PortOutPolicy( SlotRange(1) ) );
}

void LikelihoodSync::CompleteStreamInfo( ) {
    
    // we check that all incoming likelihoods have the same bin-size;
    // additionally, if the user has specified a bin-size in the config options,
    // we also (strictly) check if all bin-sizes match with the requested one
    
    double time_bin = 0;
    size_t grid_size = 0;
    unsigned int bin_size_match_counter = 1; // tallying for loop starts at one
    unsigned int grid_size_match_counter = 1; // tallying for loop starts at one
    unsigned int stream_rate_match_counter = 1; // tallying for loop starts at one
    bool can_be_finalised = true;
    decltype(data_in_port_->number_of_slots()) s;
    
    // select the time bin
    if ( time_bin_ms_ != NO_BIN_SIZE ) { // the user has specified a bin size
        time_bin = time_bin_ms_;
    } else { // if user didn't specify any bin-size, we use the one of the first slot
        time_bin = data_in_port_->streaminfo(0).datatype().time_bin_ms();
    }
    
    // check that all time bins match the selected one
    for ( s=1; s < data_in_port_->number_of_slots(); s ++ ) {
        if ( data_in_port_->streaminfo(s).datatype().time_bin_ms() == time_bin ) {
            ++ bin_size_match_counter;
        } else {
            throw_error("bin size", time_bin,
                data_in_port_->streaminfo(s).datatype().time_bin_ms(), s, name());
        }
    }
    can_be_finalised &= (bin_size_match_counter == data_in_port_->number_of_slots());
    
    LOG_IF(DEBUG, can_be_finalised) << name() << ". Bin sizes match across input slots.";
    
    // check that all grid-sizes match with each other
    grid_size = data_in_port_->streaminfo(0).datatype().grid_size();
    for ( s=1; s < data_in_port_->number_of_slots(); s ++ ) {
        if ( data_in_port_->streaminfo(s).datatype().grid_size() == grid_size ) {
            ++ grid_size_match_counter;
        } else {
            throw_error("grid size", grid_size,
                data_in_port_->streaminfo(s).datatype().grid_size(), s, name());
        }
    }
    can_be_finalised &= (grid_size_match_counter == data_in_port_->number_of_slots());
    
    // finalize time bin and grid-size
    if ( can_be_finalised ) {
        data_out_port_->streaminfo(0).datatype().Finalize( time_bin, grid_size );
    }
    
    // check that all incoming stream-rates match with each other and finalize stream-rate
    for ( s=0; s < data_in_port_->number_of_slots(); ++s ) {
        LOG(DEBUG) << name() << ". Stream rate on slot " << s << " is: " <<
            data_in_port_->streaminfo(s).stream_rate();
    }
    
    double stream_rate = data_in_port_->streaminfo(0).stream_rate();
    for ( s=1; s < data_in_port_->number_of_slots(); s ++ ) {
        if ( data_in_port_->streaminfo(s).stream_rate() == stream_rate ) {
            ++ stream_rate_match_counter;
        } else {
            throw_error("stream rate", stream_rate,
                data_in_port_->streaminfo(s).stream_rate(), s, name());
        }
    }
    if ( stream_rate_match_counter == data_in_port_->number_of_slots() ) {
        data_out_port_->streaminfo(0).Finalize( stream_rate );
    }
}

void LikelihoodSync::Prepare( GlobalContext& context ) {

    n_expected_likelihoods_ =  data_in_port_->number_of_slots();
}

void LikelihoodSync::Preprocess( ProcessingContext& context ) {
    
    n_likelihoods_received_ = 0;
    global_counter_likelihoods_synced_ = 0;
    n_synced_likelihoods_ = 1;
    
}

void LikelihoodSync::Process(ProcessingContext& context) {
    
    LikelihoodData* data_in = nullptr;
    LikelihoodData* data_out = nullptr;
    decltype(data_in_port_->number_of_slots()) s;
    uint64_t reference_timestamp;

    
    while ( !context.terminated() ) {
        
        // retrieve likelihood on 1st slot and extract reference timestamp for
        // synchronization. All "wanna-be-synced" likelihoods are expected to have
        // the same reference timestamp.
        if (!data_in_port_->slot(0)->RetrieveData( data_in )) {break;}
        reference_timestamp = data_in->hardware_timestamp();
        ++ n_likelihoods_received_;
        
        // claim output bucket and prepare it
        data_out = data_out_port_->slot(0)->ClaimData( true );
        data_out->multiply_likelihood_inplace( data_in );
        // after adding the first likelihood, we can release the data on slot 0
        data_in_port_->slot(0)->ReleaseData();
        data_out->set_hardware_timestamp( reference_timestamp );
        data_out->set_source_timestamp();
        data_out->set_time_bin( data_in->time_bin() );
        
        // check synchronization and multiply likelihoods
        for (s=1; s < data_in_port_->number_of_slots(); ++s) {
            if (!data_in_port_->slot(s)->RetrieveData( data_in )) {break;}
            ++ n_likelihoods_received_;
            
            if ( data_in->hardware_timestamp() == reference_timestamp  ) {
                data_out->multiply_likelihood_inplace( data_in );
                ++ n_synced_likelihoods_;
            } else {
                LOG(ERROR) << name() << ". Slot " << s <<
                    ". Out-of-sync likelihood received." << ". Received TS " <<
                    data_in->hardware_timestamp()
                        << " is different from reference TS "
                    << reference_timestamp << " of slot 0";
                // anything else to do in case of asynchronous likelihoods?
            } 
            data_in_port_->slot(s)->ReleaseData();
        }
        
        if ( n_synced_likelihoods_ == n_expected_likelihoods_) {
            data_out_port_->slot(0)->PublishData();
            n_synced_likelihoods_ = 1;
            ++ global_counter_likelihoods_synced_;
        }
    }  
}

void LikelihoodSync::Postprocess( ProcessingContext& context ) {
    
    auto n_non_synced = n_likelihoods_received_ -
        global_counter_likelihoods_synced_ * n_expected_likelihoods_;
    
    LOG(DEBUG) << name() << ". # received likelihoods: " <<
        n_likelihoods_received_ << ".";
    LOG(INFO) << name() << ". Streamed " << data_out_port_->slot(0)->nitems_produced()
        << " likelihoods.";
    LOG(INFO) << name() << ". # synchronized (and multiplied) likelihoods: "
        << global_counter_likelihoods_synced_ << ".";
    LOG_IF(INFO, (n_non_synced != 0)) << name() <<
        ". # incoming non-synchronized likelihoods: " << n_non_synced << ".";
    LOG_IF(INFO, (n_non_synced == 0)) << name() <<
        ". All received likelihoods were synchronized and multiplied.";
}

void LikelihoodSync::throw_error( std::string name, double value_s0,
    double current_value, SlotType s, std::string processor_name) const {
    
    throw ProcessingStreamInfoError(name + " of slot " + std::to_string(s)
        + " does not match the " + name +" of the first slot (" + name
        + " 1st slot: " + std::to_string(value_s0) + ", while this is: "
        + std::to_string(current_value) + ").",
        processor_name);
}