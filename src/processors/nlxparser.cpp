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

#include "nlxparser.hpp"

#include "g3log/src/g2log.hpp"

#include <limits>
#include <memory>

constexpr uint16_t NlxParser::MAX_NCHANNELS;
constexpr decltype(NLX_SIGNAL_SAMPLING_FREQUENCY) NlxParser::SAMPLING_PERIOD_MICROSEC;
constexpr decltype(NlxParser::delta_) NlxParser::MAX_ALLOWABLE_TIMEGAP_MICROSECONDS;
constexpr decltype(NlxParser::timestamp_) NlxParser::INVALID_TIMESTAMP;


void NlxParser::Configure( const YAML::Node & node, const GlobalContext& context ) {
    
    // number of AD channels of the system
    nchannels_ = node["nchannels"].as<decltype(nchannels_)>( DEFAULT_NCHANNELS );
    nlxrecord_.set_nchannels( nchannels_ );
    
    LOG(INFO) << name() << ". Number of channels set to " << nchannels_; 
    
     // if the network byte order should be converted to host byte order
     use_nthos_conv_ = node["convert_byte_order"].as<bool>( DEFAULT_CONVERT_BYTE_ORDER );

    
    // how many packets to pack into single multi-channel data bucket
    batch_size_ = node["batch_size"].as<decltype(batch_size_)>( DEFAULT_BATCHSIZE );
    
    // whether or not to fill missed packets with the last available sample
    gaps_filling_ = node["gaps_filling"].as<decltype(gaps_filling_)>( DEFAULT_GAPS_FILLING );
    if ( gaps_filling_!="none" and gaps_filling_!="asap" and gaps_filling_!="distributed") {
        auto msg = "Unrecognized gaps filling option (must be none, asap or distributed).";
        throw ProcessingConfigureError( msg, name() );
    }
    
    // how often updates about data stream will be sent out
    decltype(update_interval_) value = node["update_interval"].as<decltype(
        update_interval_)>(DEFAULT_UPDATE_INTERVAL_SEC);
    update_interval_ = value * NLX_SIGNAL_SAMPLING_FREQUENCY;
    if (update_interval_==0) {
        update_interval_ = std::numeric_limits<uint64_t>::max();
    }
    
    // whether or not to wait for hardware trigger to start dispatching
    hardware_trigger_ = node["hardware_trigger"].as<decltype(hardware_trigger_)>(
        DEFAULT_HARDWARE_TRIGGER );
    dispatch_ = !hardware_trigger_;
    // digital input channel to use as hardware trigger
    hardware_trigger_channel_ = node["hardware_trigger_channel"].as<decltype(
        hardware_trigger_channel_)>(DEFAULT_HARDWARE_TRIGGER_CHANNEL);
}

void NlxParser::CreatePorts() {
    
    data_in_port_ = create_input_port(
        "udp",
        VectorDataType<char>( UDP_BUFFER_SIZE ),
        PortInPolicy( SlotRange(1) ) );
    
    output_port_signal_ = create_output_port(
        "data",
        MultiChannelDataType<double>( ChannelRange(1, NlxParser::MAX_NCHANNELS) ),
        PortOutPolicy( SlotRange(1), 500 ) );
    
    output_port_ttl_ = create_output_port(
        "ttl",
        MultiChannelDataType<uint32_t>( ChannelRange(1) ),
        PortOutPolicy( SlotRange(1), 500 ) );
    
    n_invalid_ = create_writable_shared_state<int64_t>(
        "n_invalid",
        0,
        Permission::WRITE,
        Permission::NONE );
}

void NlxParser::CompleteStreamInfo() {
    
    output_port_signal_->streaminfo(0).datatype().Finalize( batch_size_, nchannels_,
        data_in_port_->slot(0)->streaminfo().stream_rate() );
    output_port_signal_->streaminfo(0).Finalize(
        data_in_port_->slot(0)->streaminfo().stream_rate() / batch_size_ );
    output_port_ttl_->streaminfo(0).datatype().Finalize( batch_size_, 1,
        data_in_port_->slot(0)->streaminfo().stream_rate() );
    output_port_ttl_->streaminfo(0).Finalize(
        data_in_port_->slot(0)->streaminfo().stream_rate() / batch_size_ );
}

void NlxParser::Prepare( GlobalContext& context ) {

    // create channel list
    channel_list_.resize( NlxParser::MAX_NCHANNELS );
    for (unsigned int i=0; i<NlxParser::MAX_NCHANNELS; i++ ) {
        channel_list_[i] = i;
    }
}

void NlxParser::Preprocess( ProcessingContext& context ) {

    sample_counter_ = batch_size_;
    valid_packet_counter_ = 0;
    
    timestamp_ = INVALID_TIMESTAMP;
    last_timestamp_ = INVALID_TIMESTAMP;
    
    stats_.clear_stats();
    n_filling_packets_ = 0;
}

void NlxParser::Process( ProcessingContext& context ) {
      
    bool update_time = false;
    unsigned int i=0;
    int b=0;
    decltype(n_filling_packets_) packets_lag = 0;
    
    VectorData<char>* data_in = nullptr;
    MultiChannelData<double>::sample_iterator data_iter;
    MultiChannelData<double>* data_out = nullptr;
    MultiChannelData<uint32_t>* ttl_data_out = nullptr;
    
    
    while ( !context.terminated() ) {
            
        if ( context.test() and roundtrip_latency_test_ ) {
            test_source_timestamps_[valid_packet_counter_] = Clock::now();
        }

        if (!data_in_port_->slot(0)->RetrieveData(data_in)) {break;}

        if ( !CheckPacket( data_in->data_array() ) ) {continue;}
        valid_packet_counter_ ++;
        
        data_in_port_->slot(0)->ReleaseData();

        if (valid_packet_counter_==1) {
            first_valid_packet_arrival_time_ = Clock::now();
            LOG(UPDATE) << name() << ". Received first valid data packet" <<
                " (TS = " << timestamp_ << ").";
        }

        if (!dispatch_) {
            LOG_IF(UPDATE, (valid_packet_counter_ == 1)) << name() <<
                ". Waiting for hardware trigger on channel "
                << hardware_trigger_channel_ << ".";
            if (nlxrecord_.parallel_port() & (1<<hardware_trigger_channel_) ) {
                dispatch_=true;
                LOG(UPDATE) << name() << ". Dispatching starts now.";
            } else { continue; }
        }

        update_time = valid_packet_counter_%update_interval_== 0;
        LOG_IF(UPDATE, update_time ) << name() << ": " <<
            valid_packet_counter_ << " packets (" <<
            valid_packet_counter_/data_in_port_->streaminfo(0).stream_rate() <<
                " s) received.";
        print_stats( update_time );

        if (sample_counter_ == batch_size_) {
            data_out = output_port_signal_->slot(0)->ClaimData(false);
            data_out->set_hardware_timestamp( timestamp_ );
            data_out->mark_as_authentic();
            ttl_data_out = output_port_ttl_->slot(0)->ClaimData(false);
            ttl_data_out->set_hardware_timestamp( timestamp_ );
            ttl_data_out->mark_as_authentic();
            sample_counter_ = 0;
        }

        // copy data from current packet onto buffer for each channel 
        data_out->set_sample_timestamp( sample_counter_, timestamp_ );
        ttl_data_out->set_sample_timestamp( sample_counter_, timestamp_ );
        data_iter = data_out->begin_sample( sample_counter_ );
        for (auto & channel : channel_list_) {
            (*data_iter) = nlxrecord_.sample_microvolt(channel);
            ++data_iter;
        }
        ttl_data_out->set_data_sample( sample_counter_, 0, nlxrecord_.parallel_port() );
        ++sample_counter_;

        if (sample_counter_ == batch_size_) {
            output_port_signal_->slot(0)->PublishData();
            output_port_ttl_->slot(0)->PublishData();
        }
        
        // stream additional packets if there were missed packets
        if ( gaps_filling_ != "none" and sample_counter_ == batch_size_ ) {
            packets_lag = stats_.n_missed - n_filling_packets_;
            if ( packets_lag >= batch_size_ ) {
                for ( b=0; b<packets_lag/batch_size_; ++b ) {
                    data_out = output_port_signal_->slot(0)->ClaimData(false);
                    LOG(DEBUG) << name() << ". mcd packet timestamp_: " << timestamp_;
                    data_out->set_hardware_timestamp( timestamp_ );
                    data_out->mark_as_duplicate();
                    ttl_data_out = output_port_ttl_->slot(0)->ClaimData(false);
                    ttl_data_out->set_hardware_timestamp( timestamp_ );
                    ttl_data_out->mark_as_duplicate();
                    LOG(DEBUG) << name() << ". mcd packet timestamp_: " << timestamp_;
                    for ( i=0; i<batch_size_; i++ ) {
                        data_out->set_sample_timestamp( i, timestamp_ );
                        ttl_data_out->set_sample_timestamp( i, timestamp_ );
                        data_iter = data_out->begin_sample( i );
                        for (auto & channel : channel_list_) {
                            (*data_iter) = nlxrecord_.sample_microvolt(channel);
                            ++data_iter;
                        }
                        ttl_data_out->set_data_sample( i, 0, nlxrecord_.parallel_port() );
                        LOG(DEBUG) << name() << ". timestamp_: " << timestamp_ << "; i=" << i;
                    }
                    output_port_signal_->slot(0)->PublishData();
                    output_port_ttl_->slot(0)->PublishData();
                    LOG( UPDATE ) << name() << ". Streamed " << batch_size_ <<
                        " duplicated samples to fill missed packets.";
                    n_filling_packets_ +=  batch_size_;
                    if (gaps_filling_ == "distributed" ) {break;}
                }
            }
        }
    }
}

void NlxParser::Postprocess( ProcessingContext& context ) {
    
    std::chrono::milliseconds runtime( 
        std::chrono::duration_cast<std::chrono::milliseconds>(
        Clock::now() - first_valid_packet_arrival_time_) );
    
    LOG(UPDATE) << name()
        << ". Finished reading : "
        << valid_packet_counter_ << " packets received over "
        << static_cast<double>(runtime.count())/1000 << " seconds at a rate of " 
        << valid_packet_counter_/static_cast<double>(runtime.count())/1000 <<
            " packets/second."; 
    print_stats();
    
    LOG(UPDATE) << name() << ". Streamed " << output_port_signal_->slot(0)->nitems_produced()
        << " multi-channel data items.";
    
    if ( context.test() and roundtrip_latency_test_ ) {
        save_source_timestamps_to_disk( valid_packet_counter_ );
    }
}

void NlxParser::print_stats( bool condition ) {
    
    LOG_IF(UPDATE, condition) << name() << ". Stats report: "
        << n_invalid_->get() <<  " invalid, " 
        << stats_.n_duplicated << " duplicated, " 
        << stats_.n_outoforder << " out of order, " 
        << stats_.n_missed << " missed, " 
        << stats_.n_gaps << " gaps. "
        << n_filling_packets_ << " packets were filled. Synchronous lag: "
        << (stats_.n_missed - n_filling_packets_)/
            data_in_port_->slot(0)->streaminfo().stream_rate() * 1e3 << " ms."; 
}

void NlxParserStats::clear_stats() {
    
    n_duplicated = 0;
    n_outoforder = 0;
    n_missed = 0;
    n_gaps = 0;
}

bool NlxParser::CheckPacket(char * buffer) {
    
    if (!nlxrecord_.FromNetworkBuffer( buffer, NLX_PACKETBYTESIZE(nchannels_), use_nthos_conv_ )) {
        n_invalid_->set( n_invalid_->get() + 1 );
        LOG(UPDATE) << name() << ": Received invalid record.";
        return false;
    }
    
    timestamp_ = nlxrecord_.timestamp();
    
    if ( last_timestamp_ == INVALID_TIMESTAMP ) {
        last_timestamp_ = timestamp_;
    } else if ( timestamp_ == last_timestamp_ ) {
        ++stats_.n_duplicated;
    } else if ( timestamp_ < last_timestamp_ ) {
        ++stats_.n_outoforder;
    } else {
        delta_ = timestamp_ - last_timestamp_;
        if ( delta_ > MAX_ALLOWABLE_TIMEGAP_MICROSECONDS ) {
            int64_t n_missed = round ( delta_ / SAMPLING_PERIOD_MICROSEC ) - 1;
            stats_.n_missed += n_missed;
            ++stats_.n_gaps;
            LOG(DEBUG) << n_missed << " timestamps were found to be missing. ";
        }
        last_timestamp_ = timestamp_;
    }
    
    return true;
}
