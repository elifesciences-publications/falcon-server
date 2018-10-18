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

#include "behaviorestimator.hpp"

#include "utilities/general.cpp"
#include "g3log/src/g2log.hpp"

void BehaviorEstimator::Configure( const YAML::Node & node, const GlobalContext& context ) {
    
    time_bin_ms_ = node["time_bin_ms"].as<decltype(time_bin_ms_)>( DEFAULT_TIME_BIN_MS );
    strict_time_bin_check_ = node["strict_time_bin_check"].as<decltype(strict_time_bin_check_)>
        ( DEFAULT_STRICT_TIME_BIN_CHECK );
    path_to_grid_ = context.resolve_path( node["path_to_grid"].as<std::string>() );
    grid_unit_s_ = node["grid_unit"].as<std::string>( );
            
    // load grid 
    if ( not path_exists( path_to_grid_ ) ) {
        throw ProcessingConfigureError( "This file does not exit: " + path_to_grid_ );
    }
    if ( load_grid_from_file( &grid_, path_to_grid_, 1, NULL ) != 0 ) {
        throw ProcessingConfigureError(
            "Cannot load the position grid from the filepath: " + path_to_grid_,
            name() ); 
    }
    LOG(UPDATE) << name() << ". Environment grid was successfully loaded.";
}

void BehaviorEstimator::CreatePorts( ) {
    
    data_in_port_ = create_input_port(
        "estimates",
        LikelihoodDataType( grid_->n_elements ),
        PortInPolicy( SlotRange(1) ) );
    
    data_out_port_ = create_output_port(
        "behavior",
        BehaviorDataType(),
        PortOutPolicy( SlotRange(1) ) ); 
}

void BehaviorEstimator::CompleteStreamInfo( ) {
    
    double incoming_bin_size =
        data_in_port_->slot(0)->streaminfo().datatype().time_bin_ms();
    
    check_buffer_sizes_and_log( incoming_bin_size, time_bin_ms_,
        strict_time_bin_check_, n_incoming_, name() );
    
    BehaviorUnit grid_unit;
    if ( grid_unit_s_ == CM_S ) {
        grid_unit = BehaviorUnit::CM;
    } else if ( grid_unit_s_ == PIXEL_S ) {
        grid_unit = BehaviorUnit::PIXEL;
    } else {
        throw ProcessingStreamInfoError( "Unrecognized behavior unit.", name() );
    }
    
    data_out_port_->streaminfo(0).datatype().Finalize( grid_unit, 1e3/time_bin_ms_ );
    
    data_out_port_->streaminfo(0).Finalize(
        data_in_port_->streaminfo(0).stream_rate() / n_incoming_ );
    
    LOG(DEBUG) << name() << ". n_incoming_ = " << n_incoming_;
}

void BehaviorEstimator::Prepare( GlobalContext& context ) {
    
    integrating_likelihood_.Initialize( grid_->n_elements );
}

void BehaviorEstimator::Process( ProcessingContext& context ) {
    
    decltype(n_incoming_) count_to_n_incoming = 0;
    LikelihoodData* data_in = nullptr;
    BehaviorData* data_out = nullptr;
    uint64_t hardware_ts = 0;
    
    while (!context.terminated()) {
        
        if (!data_in_port_->slot(0)->RetrieveData(data_in)) {break;}
        ++ counter_input_;
        
        integrating_likelihood_.accumulate_likelihood( data_in, true ); 
        data_in_port_->slot(0)->ReleaseData();
        ++ count_to_n_incoming;
        
        if ( count_to_n_incoming == 1 ) {
            hardware_ts = data_in->hardware_timestamp();
        }
        
        if ( count_to_n_incoming == n_incoming_ ) {
            
            data_out = data_out_port_->slot(0)->ClaimData(false);
            
            data_out->set_linear_position(
                grid_->elements[integrating_likelihood_.argmax()] );
            
            data_out->set_hardware_timestamp( hardware_ts );
            data_out->set_source_timestamp();
            
            data_out_port_->slot(0)->PublishData();
            integrating_likelihood_.ClearData();
            count_to_n_incoming = 0;
        }
    }
}

void BehaviorEstimator::Postprocess( ProcessingContext& context ) {
    
    LOG(INFO) << name()<< ". Streamed " << data_out_port_->slot(0)->nitems_produced()
        << " data packets.";
    LOG(DEBUG) << name() << ". # likelihoods received: " << counter_input_;
    
    counter_input_ = 0;
    integrating_likelihood_.ClearData();
}

void BehaviorEstimator::Unprepare( GlobalContext& context ) {
    
    delete_grid(&grid_); grid_ = nullptr;
}
