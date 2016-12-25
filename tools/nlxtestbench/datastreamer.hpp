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
