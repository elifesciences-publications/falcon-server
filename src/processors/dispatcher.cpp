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

#include "dispatcher.hpp"

#include "g3log/src/g2log.hpp"

#include "utilities/general.hpp"

void Dispatcher::Configure( const YAML::Node & node, const GlobalContext& context ) {
    
//    // how many packets to pack into single multi-channel data bucket
//    batch_size_ = node["batch_size"].as<decltype(batch_size_)>(DEFAULT_BATCHSIZE);
//    
    // acquisition entities : map of vector<int>
    if (node["channelmap"]) {
        channelmap_ = node["channelmap"].as<decltype(channelmap_)>();
    }
    
}

void Dispatcher::CreatePorts() {
    
    input_port_ = create_input_port(
        "data",
        MultiChannelDataType<double>( ChannelRange(1, MAX_N_CHANNELS) ),
        PortInPolicy( SlotRange(1) )
    );
    
    for (auto & it : channelmap_ ) {
        data_ports_[it.first] = create_output_port(
            it.first,
            MultiChannelDataType<double>( ChannelRange(it.second.size()) ),
            PortOutPolicy( SlotRange(1), 2000, WaitStrategy::kBlockingStrategy ) );
    }
}

void Dispatcher::CompleteStreamInfo() {
    
    incoming_batch_size_ = input_port_->streaminfo(0).datatype().nsamples();
    max_n_channels_ = input_port_->streaminfo(0).datatype().nchannels();
    LOG(INFO) << name() << ". Incoming batch size: " << incoming_batch_size_ << ".";
    for (auto & it : data_ports_ ) {
        // finalize data type with nsamples == batch_size and nchannels taken from channel map
        it.second->streaminfo(0).datatype().Finalize(
            incoming_batch_size_, channelmap_[it.first].size(),
            NLX_SIGNAL_SAMPLING_FREQUENCY );
        it.second->streaminfo(0).Finalize(
            NLX_SIGNAL_SAMPLING_FREQUENCY / incoming_batch_size_ );
    }
}


void Dispatcher::Prepare( GlobalContext& context ) {
    
    // check channel map
    for ( auto const& it : channelmap_ ) {
        for ( auto const& ch: it.second ) {
            if (ch >= max_n_channels_) {
                throw ProcessingPrepareError( "Channel "
                    +std::to_string(static_cast<int>(ch))+" is invalid", name());
            }
        }
    }
}

void Dispatcher::Preprocess( ProcessingContext& context ) {

    
}

void Dispatcher::Process( ProcessingContext& context ) {
      
    MultiChannelData<double>* data_in = nullptr;
    int port_index = 0;
    unsigned int ch, s;
//    MultiChannelData<double>::channel_iterator data_in_iter;
    std::vector<MultiChannelData<double>*> data_out_vector( data_ports_.size() );
    
    while ( !context.terminated() ) {

        // retrieve new data
        if (!input_port_->slot(0)->RetrieveData( data_in )) {break;}

        // write timestamps
        port_index = 0;
        for (auto const& it : data_ports_ ) {
            data_out_vector[port_index] = it.second->slot(0)->ClaimData(false);
            data_out_vector[port_index]->set_hardware_timestamp(
                data_in->hardware_timestamp() );
            data_out_vector[port_index]->set_source_timestamp();
            port_index++;
        }
        
        // write signals 
        port_index = 0;
        for ( auto const& it_chmap : channelmap_ ) {
            
            data_out_vector[port_index]->set_sample_timestamps(
                data_in->sample_timestamps() );
            assert(it_chmap.second.size() > 0);
            for ( ch=0; ch<it_chmap.second.size(); ch++ ) {
                for ( s=0; s<incoming_batch_size_; s++ ) {
                    data_out_vector[port_index]->set_data_sample(
                        s, ch, data_in->data_sample( s, it_chmap.second[ch] ) );
                }
            }
            port_index ++;
        }
            
        // publish data buckets
        for (auto & it: data_ports_ ) {
            it.second->slot(0)->PublishData();
        }
        input_port_->slot(0)->ReleaseData();
        
    }//while
    
}

void Dispatcher::Postprocess( ProcessingContext& context ) {
    
    SlotType s;
    for (auto & it : data_ports_ ) {
        for (s=0; s < it.second->number_of_slots(); ++s) {
            LOG(INFO) << name()<< ". Port " << it.first << ". Slot " << s <<
                ". Streamed " << it.second->slot(s)->nitems_produced() <<
                " data packets. ";
        }
    }
}
