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

#include <cstring>
#include <iostream>

#include <unistd.h>

#include "datastreamer.hpp"

#include "utilities/time.hpp"


void busysleep_until( TimePoint t ) {
    
    while ( Clock::now() < t ) {}
}

DataStreamer::DataStreamer( DataSource * source, double rate, std::string ip, int port, uint64_t npackets ) : 
    source_(source), rate_(rate), ip_(ip), port_(port), max_packets_(npackets) {
        
    /*create UDP socket*/   
    if ( (udp_socket_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        throw std::runtime_error( "Unable to create socket." );
    }
    
    memset((char *)&server_address_, 0, sizeof(server_address_));
    server_address_.sin_family = AF_INET;
    server_address_.sin_addr.s_addr = inet_addr(ip_.c_str());
    server_address_.sin_port = htons(port_);
    
    if (max_packets_==0) {
        max_packets_ = std::numeric_limits<uint64_t>::max();
    }
    
}
    
DataStreamer::~DataStreamer() {
    close(udp_socket_);
}
    
bool DataStreamer::running() const {
    
    return running_;
}

bool DataStreamer::terminated() const {
    
    return terminate_;
}

void DataStreamer::Terminate() {
    
    terminate_ = true;
}

void DataStreamer::Run() {
    
    uint64_t npackets = 0;
    
    TimePoint start;
    std::chrono::microseconds period( (uint64_t) (1000000./rate_) );
            
    char* buffer;
    
    std::cout << "Started streaming at " << std::to_string(rate_)
                  << " Hz: " << source_->string() << std::endl;
    
    auto begin_time = std::chrono::high_resolution_clock::now();
    
    while ( !terminated() && npackets<max_packets_ ) {
        
        start = Clock::now();
        
        if (!source_->Produce( &buffer )) { break; }
        
        busysleep_until( start + period );
        
        if ( sendto( udp_socket_, buffer, BUFFERSIZE, 0,
                     (struct sockaddr*) &server_address_, sizeof(server_address_) ) != BUFFERSIZE ) {
            std::cout << "Error sending data packet." << std::endl;
            break;
        }
        
        npackets++;
        
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( end_time - begin_time ).count();
        
    std::cout << "Finished streaming: " << source_->string() << std::endl;
    
    std::cout << "Number of packets sent = " << npackets << " in "
              << static_cast<double>(duration/1000.) << " seconds ( average rate = " 
              << static_cast<double>(npackets*1000./duration) << " Hz )" << std::endl << std::endl;
    
    Terminate();
    
} 
    
void DataStreamer::Start() {
    if ( !running() ) {
        terminate_ = false;
        thread_ = std::thread( &DataStreamer::Run, this );
        running_ = true;
    }
}
    
void DataStreamer::Stop() {
    if ( running() ) {
        if (!terminated()) {
            Terminate();
        }
        thread_.join();
        terminate_ = false;
        running_ = false;
    }
    
}

void DataStreamer::set_source( DataSource * source ) {
    if (!running()) {
        source_ = source;
    }
}
