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

#include "rebuffer.hpp"

#include "g3log/src/g2log.hpp"
#include "utilities/time.hpp"


constexpr decltype(Rebuffer::downsample_factor_) Rebuffer::DEFAULT_DOWNSAMPLE_FACTOR;
constexpr decltype(Rebuffer::buffer_size_samples_) Rebuffer::DEFAULT_BUFFER_SIZE_SAMPLES;
constexpr decltype(Rebuffer::buffer_size_seconds_) Rebuffer::DEFAULT_BUFFER_SIZE_SECONDS;

void Rebuffer::CreatePorts( ) {
    
    data_in_port_ = create_input_port(
        "data",
        MultiChannelDataType<double>( ChannelRange(1,256) ),
        PortInPolicy( SlotRange(0,256) ) );
    
    data_out_port_ = create_output_port(
        "data",
        MultiChannelDataType<double>( ChannelRange(1,256) ),
        PortOutPolicy( SlotRange(0,256) ) );
}

void Rebuffer::Configure( const YAML::Node & node, const GlobalContext& context ) {
    
    downsample_factor_ = node["downsample_factor"].as<decltype(downsample_factor_)>
        ( DEFAULT_DOWNSAMPLE_FACTOR );
    if (downsample_factor_==0) {
        throw ProcessingConfigureError(
            "Downsample factor should be larger than zero.", name());
    }
    LOG( INFO ) << name() << ". Downsample factor set to " << downsample_factor_ << ".";
    
    buffer_unit_ = node["buffer_unit"].as<decltype(buffer_unit_)>( DEFAULT_BUFFER_UNIT );
    if (buffer_unit_ == "samples") {
        buffer_size_samples_ = node["buffer_size"].as<decltype(buffer_size_samples_)>
            ( DEFAULT_BUFFER_SIZE_SAMPLES );
        LOG( INFO ) << name() << ". Buffer size set to " << buffer_size_samples_
            << " samples.";
    } else if (buffer_unit_ == "seconds") {
        buffer_size_seconds_ = node["buffer_size"].as<decltype(buffer_size_seconds_)>
            ( DEFAULT_BUFFER_SIZE_SECONDS );
        if (buffer_size_seconds_ < 0) {
            throw ProcessingConfigureError(
                "Buffer size should be equal to or larger than zero.", name());
        }
        LOG( INFO ) << name() << ". Buffer size set to " << buffer_size_seconds_
            << " seconds.";
    } else {
        throw ProcessingConfigureError("Invalid buffer_unit value.", name());
    }
}

void Rebuffer::CompleteStreamInfo( ) {
    
    // check if we have the same number of input and output slots
    if (data_in_port_->number_of_slots() != data_out_port_->number_of_slots()) {
        throw ProcessingStreamInfoError(
            "Number of outputs does not match the number of inputs.", name() );
    }
    
    // compute number of output samples for each input stream
    if (buffer_unit_=="samples" && buffer_size_samples_>0) {
        buffer_size_.assign( data_in_port_->number_of_slots(), buffer_size_samples_ );
    } else if (buffer_unit_=="seconds" && buffer_size_seconds_>0) {
        buffer_size_.assign( data_in_port_->number_of_slots(), 0 );
        for ( int k=0; k<data_in_port_->number_of_slots(); ++k ) {
            buffer_size_[k] = time2samples<decltype(buffer_size_samples_)>(
                buffer_size_seconds_,
                data_in_port_->streaminfo(k).datatype().sample_rate() / downsample_factor_ );
            if (buffer_size_[k]==0) {
                throw ProcessingStreamInfoError( "Buffer duration is zero.", name());
            }
        }
    } else {
        for ( int k=0; k<data_in_port_->number_of_slots(); ++k ) {
            buffer_size_[k] = std::max( 1u, static_cast<decltype(buffer_size_samples_)>(
                std::floor(
                    data_in_port_->streaminfo(k).datatype().nsamples() / downsample_factor_ ) ) );
        }
    }
    
    // finalize
    for ( int k=0; k<data_in_port_->number_of_slots(); ++k ) {
        data_out_port_->streaminfo(k).datatype().Finalize(
            buffer_size_[k], data_in_port_->streaminfo(k).datatype().nchannels(),
            data_in_port_->streaminfo(k).datatype().sample_rate() / downsample_factor_ );
        data_out_port_->streaminfo(k).Finalize(
            data_in_port_->streaminfo(k).stream_rate() *
                data_in_port_->streaminfo(k).datatype().nsamples() / buffer_size_[k] );
    }
    
}

void Rebuffer::Process( ProcessingContext& context ) {
    
    auto nslots = data_in_port_->number_of_slots();
    
    MultiChannelData<double>* data_in = nullptr;
    std::vector<MultiChannelData<double>*> data_out;
    data_out.assign(nslots, nullptr);

    decltype(buffer_size_) sample_out_counter = buffer_size_;
    decltype(buffer_size_) offset;
    offset.assign( nslots, 0);
    
    unsigned int s=0;
    
    while ( !context.terminated() ) {
        
        // go through all slots
        for (int k=0; k<nslots; ++k) {
            
            // retrieve new data
            if (!data_in_port_->slot(k)->RetrieveData( data_in )) {break;}
            
            s = 0;
            
            while ( s < data_in->nsamples() ) {
                
                for ( s = offset[k];
                s<data_in->nsamples() && sample_out_counter[k]<buffer_size_[k];
                s+=downsample_factor_) {
                    for (unsigned int c=0 ; c<data_in->nchannels(); ++c) {
                        data_out[k]->set_data_sample( sample_out_counter[k], c,
                            data_in->data_sample(s,c) );
                    }
                    data_out[k]->set_sample_timestamp( sample_out_counter[k],
                        data_in->sample_timestamp(s) );
                    sample_out_counter[k]++;
                }
                
                if ( sample_out_counter[k] == buffer_size_[k] ) {
                    data_out_port_->slot(k)->PublishData();
                    data_out[k] = data_out_port_->slot(k)->ClaimData(false);
                    data_out[k]->CloneTimestamps( *data_in );
                    sample_out_counter[k] = 0;
                }
                
                if ( s >= data_in->nsamples() ) {
                    offset[k] = s - data_in->nsamples();
                } else {
                    offset[k] = s;
                }
                
            }
            
            data_in_port_->slot(k)->ReleaseData();
            
        }
                
    }
}
