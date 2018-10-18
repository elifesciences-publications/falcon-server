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

/* NlxPureReader: reads raw data of a Neuralynx Digilynx data acquisition 
 * on a UDP buffer
 * 
 * input ports:
 * none
 *
 * output ports:
 * udp <VectorData<char>> (1 slot)
 *
 * exposed states:
 * none
 *
 * exposed methods:
 * none
 *
 * options:
 * address <string> - IP address of Digilynx system
 * port <unsigned int> - port of Digilynx system
 * npackets <uint64_t> - number of raw data packets to read before
 *   exiting (0 = continuous streaming)
 * 
 */

#ifndef NLXPUREREADER_HPP
#define NLXPUREREADER_HPP

#include "../graph/iprocessor.hpp"
#include "../data/vectordata.hpp"

#include "neuralynx/nlx.hpp"
#include "utilities/time.hpp"

#include <limits>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>


class NlxPureReader : public IProcessor {
    
public:
    NlxPureReader() : IProcessor( PRIORITY_MAX ) {};
    
    virtual void Configure( const YAML::Node  & node, const GlobalContext& context ) override;
    virtual void CreatePorts() override;
    virtual void CompleteStreamInfo() override;
    virtual void Prepare( GlobalContext& context ) override;
    virtual void Preprocess( ProcessingContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;
    
public:
    static constexpr uint16_t MAX_NCHANNELS = 128;
    static constexpr decltype(MAX_NCHANNELS) UDP_BUFFER_SIZE =
        NLX_PACKETBYTESIZE(MAX_NCHANNELS);
    
// config options
protected:
    std::string address_;
    unsigned int port_;
    std::uint64_t npackets_;

// internals
protected:
    PortOut<VectorDataType<char>>* output_port_;
    WritableState<int64_t>* n_invalid_;
    
    fd_set file_descriptor_set_;
    int udp_socket_;
    int udp_socket_select_;
    struct sockaddr_in server_addr_; 
    
    decltype(npackets_) valid_packet_counter_;
    struct timeval timeout_;
    TimePoint first_valid_packet_arrival_time_;
    
    ssize_t size_;
    int recvlen_;
    VectorData<char>* data_out_;
    
public:
    static constexpr decltype(NLX_SIGNAL_SAMPLING_FREQUENCY)
        SAMPLING_PERIOD_MICROSEC = 1e6 / NLX_SIGNAL_SAMPLING_FREQUENCY;
    const std::string DEFAULT_ADDRESS = "127.0.0.1"; //testbench
    const decltype(port_) DEFAULT_PORT = 5000;
    const decltype(npackets_) DEFAULT_NPACKETS = 0;
    const decltype(timeout_.tv_sec) TIMEOUT_SEC = 3;
  
};

#endif // nlxpurereader.hpp
