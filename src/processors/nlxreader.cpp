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

#include "nlxreader.hpp"

#include "g3log/src/g2log.hpp"

#include <limits>
#include <chrono>

constexpr uint16_t NlxReader::MAX_NCHANNELS;
constexpr decltype(NlxReader::MAX_NCHANNELS) NlxReader::UDP_BUFFER_SIZE;
constexpr decltype(NLX_SIGNAL_SAMPLING_FREQUENCY) NlxReader::SAMPLING_PERIOD_MICROSEC;
constexpr decltype(NlxReader::delta_) NlxReader::MAX_ALLOWABLE_TIMEGAP_MICROSECONDS;
constexpr decltype(NlxReader::timestamp_) NlxReader::INVALID_TIMESTAMP;

void NlxReaderStats::clear_stats() {
    
    n_invalid = 0;
    n_duplicated = 0;
    n_outoforder = 0;
    n_missed = 0;
    n_gaps = 0;
}

bool NlxReader::CheckPacket(char * buffer, int recvlen) {
    
    if (!nlxrecord_.FromNetworkBuffer( buffer_, recvlen )) {
        ++stats_.n_invalid;
        LOG(INFO) << name() << ": Received invalid record.";
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

void NlxReader::CreatePorts() {
    
    for (auto & it : channelmap_ ) {
        data_ports_[it.first] = create_output_port(
            it.first,
            MultiChannelDataType<double>( ChannelRange(it.second.size()) ),
            PortOutPolicy( SlotRange(1), 500, WaitStrategy::kBlockingStrategy ) );
    }
}

void NlxReader::CompleteStreamInfo() {
    
    for (auto & it : data_ports_ ) {
        // finalize data type with nsamples == batch_size and nchannels taken from channel map
        it.second->streaminfo(0).datatype().Finalize(
            batch_size_, channelmap_[it.first].size(), NLX_SIGNAL_SAMPLING_FREQUENCY );
        it.second->streaminfo(0).Finalize( NLX_SIGNAL_SAMPLING_FREQUENCY / batch_size_ );
    }
}

void NlxReader::Configure( const YAML::Node & node, const GlobalContext& context ) {
    
    // ip address : string
    address_ = node["address"].as<decltype(address_)>(DEFAULT_ADDRESS);
    // port : unsigned int
    port_ = node["port"].as<decltype(port_)>(DEFAULT_PORT);
    
    // acquisition entities : map of vector<int>
    if (node["channelmap"]) {
        channelmap_ = node["channelmap"].as<decltype(channelmap_)>();
    }
    
    // npackets : int (number of packets to read, 0 means continuous recording)
    npackets_ = node["npackets"].as<decltype(npackets_)>(DEFAULT_NPACKETS);
    if (npackets_==0) {
        npackets_ = std::numeric_limits<decltype(npackets_)>::max();
    }
    
    // how many packets to pack into single multi-channel data bucket
    batch_size_ = node["batch_size"].as<decltype(batch_size_)>(DEFAULT_BATCHSIZE);
    
    // number of AD channels of the system
    nchannels_ = node["nchannels"].as<decltype(nchannels_)>(DEFAULT_NCHANNELS);
    nlxrecord_.set_nchannels( nchannels_ );
    
    // how often updates about data stream will be sent out
    decltype(update_interval_) value = node["update_interval"].as<decltype(
        update_interval_)>(DEFAULT_UPDATE_INTERVAL_SEC);
    update_interval_ = value * NLX_SIGNAL_SAMPLING_FREQUENCY;
    if (update_interval_==0) {
        update_interval_ = std::numeric_limits<uint64_t>::max();
    }
    
    // whether or not to wait for hardware trigger to start dispatching
    hardware_trigger_ = node["hardware_trigger"].as<decltype(hardware_trigger_)>(
        DEFAULT_HARDWARE_TRIGGER);
    dispatch_ = !hardware_trigger_;
    
    // digital input channel to use as hardware trigger
    hardware_trigger_channel_ = node["hardware_trigger_channel"].as<decltype(
        hardware_trigger_channel_)>(DEFAULT_HARDWARE_TRIGGER_CHANNEL);
    
}

void NlxReader::Prepare( GlobalContext& context ) {
    
    memset((char *)&server_addr_, 0, sizeof(server_addr_));
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = inet_addr(address_.c_str());
    server_addr_.sin_port = htons(port_);
}

void NlxReader::Preprocess( ProcessingContext& context ) {

    sample_counter_ = batch_size_;
    valid_packet_counter_ = 0;
    const int y = 1;
    
    timestamp_ = INVALID_TIMESTAMP;
    last_timestamp_ = INVALID_TIMESTAMP;
    
    stats_.clear_stats();
    
    if ( context.test() ) {
        prepare_latency_test( context );
    }
    
    sleep(1); // reduces probability of missed packets when connecting to ongoing stream
    
    if ( (udp_socket_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        throw ProcessingPrepareError( "Unable to create socket.", name() );
    }
    LOG(UPDATE) << name() << ". Socket created.";
    setsockopt(udp_socket_, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
    if ( bind(udp_socket_, (struct sockaddr *)&server_addr_, sizeof(server_addr_)) < 0 ) {
        throw ProcessingPreprocessingError( "Socket binding failed.", name() );
    }
    LOG(UPDATE) << name() << ". Socket binding successful.";
}

void NlxReader::Process( ProcessingContext& context ) {
      
    bool update_time = false;
    int data_index = 0;
    MultiChannelData<double>::sample_iterator data_iter;
    std::vector<MultiChannelData<double>*> data_vector(data_ports_.size());
    
    while ( !context.terminated() && valid_packet_counter_<npackets_ ) {
        
        // check if packets have arrived (with time-out)
        FD_ZERO (&file_descriptor_set_); //clear the file descriptor set
        FD_SET  (udp_socket_, &file_descriptor_set_); //add the socket (it's basically acting like a filedescriptor) to the set
        
        // set time-out
        timeout_.tv_sec = TIMEOUT_SEC;
        timeout_.tv_usec = 0;
        
        // packets available?
        ssize_t size = select(udp_socket_+1, &file_descriptor_set_, 0, 0, &timeout_);
        
        if (size == 0) {
            LOG(DEBUG) << name() << ": Timed out waiting for data. Connection lost?";
            continue;
		}
		
        if (size == -1) {
            LOG(DEBUG) << name() << ": Select error on UDP socket.";
            continue;
        }
		
        if (size > 0) { // receive packet
            
            if ( context.test() ) {
                test_source_timestamps_[valid_packet_counter_] = Clock::now();
            }
            
            int recvlen = recvfrom(udp_socket_, buffer_, UDP_BUFFER_SIZE, 0, NULL, NULL);
            
            if (!CheckPacket( buffer_, recvlen )) { continue; }
            
            valid_packet_counter_++;
            
            if (valid_packet_counter_==1) {
                first_valid_packet_arrival_time_ = Clock::now();
                LOG(UPDATE) << name() << ": Received first valid data packet" <<
                    " (TS = " << nlxrecord_.timestamp() << ").";
            }
            
            update_time = valid_packet_counter_%update_interval_== 0;
            LOG_IF(UPDATE, update_time ) << name() << ": " <<
                valid_packet_counter_ << " packets (" <<
                valid_packet_counter_/NLX_SIGNAL_SAMPLING_FREQUENCY << " s) received.";
            print_stats( update_time );
            
            if (!dispatch_) {
                LOG_IF(UPDATE, (valid_packet_counter_ == 1)) << name() <<
                    ". Waiting for hardware trigger on channel "
                    << hardware_trigger_channel_ << ".";
                if (nlxrecord_.parallel_port() & (1<<hardware_trigger_channel_) ) {
                    dispatch_=true;
                    LOG(UPDATE) << name() << ". Dispatching starts now.";
                } else { continue; }
            }
            
            // claim new data buckets
            if (sample_counter_ == batch_size_) {
                data_index = 0;
                for (auto & it : data_ports_ ) {
                    data_vector[data_index] = it.second->slot(0)->ClaimData(false);
                    // set data bucket metadata
                    data_vector[data_index]->set_hardware_timestamp(
                        nlxrecord_.timestamp() );
                    data_vector[data_index]->set_source_timestamp();
                    data_index++;
                }
                sample_counter_ = 0;
            }
                        
            // copy data onto buffers for each configured channel group
            data_index = 0;
            for (auto & it : channelmap_ ) {
                data_vector[data_index]->set_sample_timestamp(
                    sample_counter_, nlxrecord_.timestamp() );
                data_iter = data_vector[data_index]->begin_sample( sample_counter_ );
                for ( auto & channel : it.second ) {
                    (*data_iter) = nlxrecord_.sample_microvolt(channel);
                    ++data_iter;
                }
                data_index++;
            }
            
            ++sample_counter_;
            
            // publish data buckets
            if (sample_counter_ == batch_size_) {
                for (auto & it : data_ports_ ) {
                    it.second->slot(0)->PublishData();
                }
            }
            
        } // receive packet
        
    }//while
    
    
    SlotType s;
    for (auto & it : data_ports_ ) {
        for (s=0; s < it.second->number_of_slots(); ++s) {
            LOG(INFO) << name()<< ". Port " << it.first << ". Slot " << s <<
                ". Streamed " << it.second->slot(s)->nitems_produced() <<
                " data packets. ";
        }
    }
}

void NlxReader::Postprocess( ProcessingContext& context ) {
    
    LOG_IF(UPDATE, (valid_packet_counter_ == npackets_) ) << 
        "Requested number of packets was read. You can now STOP processing.";
    
    std::chrono::milliseconds runtime( 
        std::chrono::duration_cast<std::chrono::milliseconds>(
        Clock::now() - first_valid_packet_arrival_time_) );
    
    LOG(UPDATE) << name()
        << ". Finished reading : "
        << valid_packet_counter_ << " packets received over "
        << static_cast<double>(runtime.count())/1000 << " seconds at a rate of " 
        << valid_packet_counter_/static_cast<double>(runtime.count())/1000 << " packets/second."; 
    print_stats();
    
    close( udp_socket_ );
    
    if ( context.test() ) {
        save_source_timestamps_to_disk( valid_packet_counter_ );
    }
}

void NlxReader::print_stats( bool condition ) {
    
    LOG_IF(UPDATE, condition) << name() << ". Stats report: "
        << stats_.n_invalid <<  " invalid, " 
        << stats_.n_duplicated << " duplicated, " 
        << stats_.n_outoforder << " out of order, " 
        << stats_.n_missed << " missed, " 
        << stats_.n_gaps << " gaps).";
}
