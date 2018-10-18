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

/* NlxParser: reads raw data from a Neuralynx Digilynx data acquisition 
 * system and turns it a single MultiChannelData output stream of all available
 * channel
 * 
 * input ports:
 * udp <VectorData<char>> (1 slot)
 *
 * output ports:
 * data <MultiChannelData> (1 slot)
 * ttl <MultiChannelData> (1 slot)
 *
 * exposed states:
 * none
 *
 * exposed methods:
 * none
 *
 * options:
 * batch_size <unsigned int> - how many samples to pack into single
 *   MultiChannelData bucket
 * npackets <uint64_t> - number of raw data packets to read before
 *   exiting (0 = continuous streaming)
 * update_interval <unsigned int> - time interval (in seconds) between
 *   log updates
 * gaps_filling <string> - if "none", no filling of missed packets; if "asap"
 * all missed packets will be filled with last available batch of samples;
 * if "distributed" missed packets will be filled with the last available
 * batch of samples at each iteration.
 * hardware_trigger <bool> - enable use of hardware triggered dispatching
 * hardware_trigger_channel <uint8> - which DIO channel to use as trigger
 * 
 */

#ifndef NLXPARSER_HPP
#define NLXPARSER_HPP

#include "../graph/iprocessor.hpp"
#include "../data/vectordata.hpp"
#include "../data/multichanneldata.hpp"

#include "neuralynx/nlx.hpp"
#include "utilities/time.hpp"

#include <limits>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>


struct NlxParserStats {
    int64_t n_duplicated;
    int64_t n_outoforder;
    int64_t n_missed;
    int64_t n_gaps;
    
    void clear_stats();
};

class NlxParser : public IProcessor {
    
public:
    NlxParser() : IProcessor( PRIORITY_HIGH ) {};
    
    virtual void Configure( const YAML::Node  & node, const GlobalContext& context ) override;
    virtual void CreatePorts() override;
    virtual void CompleteStreamInfo() override;
    virtual void Prepare( GlobalContext& context ) override;
    virtual void Preprocess( ProcessingContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;
      
protected:
    bool CheckPacket(char * buffer);
    void print_stats( bool condition=true );
    
public:
    static constexpr uint16_t MAX_NCHANNELS = 128;
    static constexpr decltype(MAX_NCHANNELS) UDP_BUFFER_SIZE =
        NLX_PACKETBYTESIZE(MAX_NCHANNELS);
    
// config options
protected:
    unsigned int batch_size_;
    unsigned int nchannels_;
    std::string gaps_filling_;

// internals
protected:
    PortOut<MultiChannelDataType<double>>* output_port_signal_;
    PortOut<MultiChannelDataType<uint32_t>>* output_port_ttl_;
    PortIn<VectorDataType<char>>* data_in_port_;
    WritableState<int64_t>* n_invalid_; 
    
    unsigned int sample_counter_;
    uint64_t valid_packet_counter_;
    
    TimePoint first_valid_packet_arrival_time_;
    
    uint64_t timestamp_;
    decltype(timestamp_) last_timestamp_;
    
    NlxSignalRecord nlxrecord_;
    
    decltype(valid_packet_counter_) update_interval_;
    
    NlxParserStats stats_;
    decltype(timestamp_) delta_;
    
    std::vector<unsigned int> channel_list_;
    
    bool dispatch_;
    bool hardware_trigger_;
    bool use_nthos_conv_;
    uint32_t hardware_trigger_channel_;
    
    int64_t n_filling_packets_;
    
public:
    static constexpr decltype(NLX_SIGNAL_SAMPLING_FREQUENCY)
        SAMPLING_PERIOD_MICROSEC = 1e6 / NLX_SIGNAL_SAMPLING_FREQUENCY;
    const decltype(batch_size_) DEFAULT_BATCHSIZE = 2;
    const decltype(nchannels_) DEFAULT_NCHANNELS = 128;
    const bool DEFAULT_CONVERT_BYTE_ORDER = true;
    const decltype(gaps_filling_) DEFAULT_GAPS_FILLING = "asap";
    const decltype(update_interval_) DEFAULT_UPDATE_INTERVAL_SEC = 20;
    const decltype(hardware_trigger_) DEFAULT_HARDWARE_TRIGGER = false;
    const decltype(hardware_trigger_channel_) DEFAULT_HARDWARE_TRIGGER_CHANNEL = 0;
    static constexpr decltype(delta_) MAX_ALLOWABLE_TIMEGAP_MICROSECONDS =
        trunc( SAMPLING_PERIOD_MICROSEC ) + 1;
    static constexpr decltype(timestamp_) INVALID_TIMESTAMP =
        std::numeric_limits<decltype(timestamp_)>::max();
};

#endif // nlxparser.hpp
