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

#include "runningstats.hpp"
#include "g3log/src/g2log.hpp"


void RunningStats::CreatePorts( ) {
    
    data_in_port_ = create_input_port( "data", MultiChannelDataType<double>( ChannelRange(1,256) ), PortInPolicy( SlotRange(1) ) );
    data_out_port_ = create_output_port( "data", MultiChannelDataType<double>( ChannelRange(1,256) ), PortOutPolicy( SlotRange(1) ) );
    
}

void RunningStats::Configure( const YAML::Node & node, const GlobalContext& context ) {
    
    integration_time_ = node["integration_time"].as<double>(DEFAULT_INTEGRATION_TIME);
    
    outlier_protection_ = node["outlier_protection"].as<bool>(DEFAULT_OUTLIER_PROTECTION);
    outlier_zscore_ = node["outlier_zscore"].as<double>(DEFAULT_OUTLIER_ZSCORE);
    outlier_half_life_ = node["outlier_half_life"].as<double>(DEFAULT_OUTLIER_HALF_LIFE);
    
}

void RunningStats::CompleteStreamInfo( ) {
       
    for ( int k=0; k<data_in_port_->number_of_slots(); ++k ) {
        data_out_port_->streaminfo(k).datatype().Finalize( data_in_port_->streaminfo(k).datatype() );
        data_out_port_->streaminfo(k).Finalize( data_in_port_->streaminfo(k).stream_rate() );
    }
    
}

void RunningStats::Preprocess( ProcessingContext& context ) {
    
    double sample_rate = data_in_port_->slot(0)->streaminfo().datatype().sample_rate();
    double alpha = 1.0 / (integration_time_ * sample_rate);
    
    stats_.reset( new dsp::algorithms::RunningMeanMAD( alpha, integration_time_*sample_rate, outlier_protection_, outlier_zscore_, outlier_half_life_ ) );
    
}

void RunningStats::Process( ProcessingContext& context ) {
    
    MultiChannelData<double>* data_in;
    MultiChannelData<double>* data_out;
    
    unsigned N = 100;
    
    while (!context.terminated()) {
        
        // retrieve new data
        if (!data_in_port_->slot(0)->RetrieveData( data_in )) {break;}
        
        data_out = data_out_port_->slot(0)->ClaimData(false);
        
        // loop through each sample
        for ( unsigned int s=0; s<data_in->nsamples(); ++s ) {
            
            stats_->add_sample( data_in->data_sample(s,0) );
            
            data_out->set_data_sample( s, 0, stats_->center() );
            data_out->set_data_sample( s, 1, stats_->dispersion() );
            data_out->set_sample_timestamp( s, data_in->sample_timestamp(s) );
            
        }
        
        data_out->CloneTimestamps( *data_in );
        
        data_out_port_->slot(0)->PublishData();
        data_in_port_->slot(0)->ReleaseData();
        
        N--;
        if (N==0) {
            LOG(UPDATE) << "center = " << stats_->center() << ", dispersion = " << stats_->dispersion();
            N = 100;
        }
        
    }
}
