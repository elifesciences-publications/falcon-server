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

#ifndef CONNECTIONS_H
#define CONNECTIONS_H

#include <string>

#include "processorengine.hpp"
#include "../data/idata.hpp"

class IProcessor;
class IPortIn;
class IPortOut;
class ISlotIn;
class ISlotOut;

class PortAddress {
public:
    PortAddress( std::string processor, std::string port ) :
    processor_name_(processor), port_name_(port) {}
    
    const std::string processor() const { return processor_name_; }
    const std::string port() const {return port_name_; }
    
    void set_port( std::string port ) { port_name_ = port; }
    
    const std::string string() const;
    
protected:
    std::string processor_name_;
    std::string port_name_;    
};

class SlotAddress : public PortAddress {
public:
    SlotAddress( std::string processor, std::string port, int slot ) :
    PortAddress( processor, port ), slot_(slot) {}
    
    int slot() const { return slot_; }
    
    void set_slot( int slot ) { slot_ = slot; }
    
    const std::string string() const;
    
protected:
    int slot_;
};

//forward declaration
class StreamInConnector;

class StreamOutConnector {
public:
    StreamOutConnector( SlotAddress address, const ProcessorEngineMap& processors );
    
    IProcessor* processor();
    IPortOut* port();
    ISlotOut* slot();
    
    void Connect( StreamInConnector* downstream );
    
    const SlotAddress& address() const { return address_; }
    
    std::string string() const { return address_.string(); }
    
    IStreamInfo& streaminfo();
    
protected:
    SlotAddress address_;
    ProcessorEngine * processor_engine_=nullptr; //observing pointer (no ownership)
};

class StreamInConnector {
public:
    StreamInConnector( SlotAddress address, const ProcessorEngineMap& processors );

    IProcessor* processor();
    IPortIn* port();
    ISlotIn* slot();
    
    void Connect( StreamOutConnector* upstream );
    
    bool CheckCompatibility( StreamOutConnector* upstream );
    
    const SlotAddress& address() const { return address_; }
    
    std::string string() const { return address_.string(); }
    
protected:
    SlotAddress address_;
    ProcessorEngine * processor_engine_=nullptr; //observing pointer (no ownership)
};

class StreamConnection {
public:
    StreamConnection( SlotAddress out, SlotAddress in ) :
    out_(out), in_(in), connected_(false) {}
    
    bool connected() const { return connected_.load(); }
    
    void Connect( const ProcessorEngineMap& processors );
    
    std::string string() const {
        std::string s;
        if (connected()) {
            s += out_connector_->string() + "=" + in_connector_->string();
        } else {
            s += out_.string() + "=" + in_.string();
        }
        return s;
    }
    
protected:
    SlotAddress out_;
    SlotAddress in_;
    
    std::unique_ptr<StreamOutConnector> out_connector_;
    std::unique_ptr<StreamInConnector> in_connector_;
    
    std::atomic<bool> connected_;
};


#endif
