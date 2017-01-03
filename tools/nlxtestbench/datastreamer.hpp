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

#ifndef DATASTREAMER_H
#define DATASTREAMER_H

#include <string>
#include <thread>

#include <chrono>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.hpp"
#include "datasource.hpp"


void busysleep_until( std::chrono::time_point<std::chrono::high_resolution_clock> t );

class DataStreamer {
    
public:
    DataStreamer( DataSource * source, double rate, std::string ip, int port, uint64_t npackets );
    ~DataStreamer();
    
    bool running() const;
    bool terminated() const;
    void Terminate();
    
    void Run();    
    void Start();
    void Stop();

    void set_source( DataSource * source );

protected:
    
    std::thread thread_;
    bool running_ = false;
    bool terminate_ = false;
    
    DataSource* source_;
    double rate_;
    std::string ip_;
    int port_;
    uint64_t max_packets_;
    
    struct sockaddr_in server_address_;
    int udp_socket_;
    
};

#endif // DATASTREAMER_H
