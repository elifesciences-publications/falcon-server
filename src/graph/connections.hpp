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
