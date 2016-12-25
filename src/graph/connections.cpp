#include "connections.hpp"
#include "iprocessor.hpp"
#include "istreamports.hpp"


const std::string PortAddress::string() const {
    
    std::string s;
    s = processor() + "." + port();
    return s;
}

const std::string SlotAddress::string() const {
    
    std::string s;
    s = processor() + "." + port() + "." + std::to_string( slot() );
    return s;
}

StreamOutConnector::StreamOutConnector( SlotAddress address, const ProcessorEngineMap& processors ) : address_(address) {
    
    // get processor
    try {
        processor_engine_ = processors.at( address_.processor() ).second.get();
    } catch (std::out_of_range& e) {
        throw std::out_of_range( "Unknown processor \"" + address_.processor() + "\"" );
    }
    
    // get default port if needed
    if (address_.port()=="") {
        address_.set_port( processor()->default_output_port() );
    }
    
    // test if port exists
    if (!processor()->has_output_port( address_.port() ) ) {
        throw std::out_of_range( "Unknown output port \"" + address_.port() + "\" on processor \"" + address_.processor() + "\"." );
    }
    
    // test if slot is valid and create new one if necessary
    int slot = port()->ReserveSlot( address_.slot() );
    
    // and update slot in address
    if (slot<0) {
        throw std::out_of_range( "Unable to reserve slot \"" + std::to_string(address_.slot()) + "\" for output port \"" + address_.processor() + "." + address_.port() + "\"." );
    }
    
    address_.set_slot( slot );
    
}

IProcessor* StreamOutConnector::processor() { return processor_engine_->processor(); }
IPortOut* StreamOutConnector::port() { return processor()->output_port( address_.port() ); }
ISlotOut* StreamOutConnector::slot() { return port()->slot( address_.slot() ); }

void StreamOutConnector::Connect( StreamInConnector* downstream ) {
    
    port()->Connect( address_.slot(), downstream );
}

IStreamInfo& StreamOutConnector::streaminfo() {
    
    processor()->NegotiateConnections();
    return slot()->streaminfo();
}

StreamInConnector::StreamInConnector( SlotAddress address, const ProcessorEngineMap& processors ) : address_(address) {
    
    // get processor
    try {
        processor_engine_ = processors.at( address_.processor() ).second.get();
    } catch (std::out_of_range& e) {
        throw std::out_of_range( "Unknown processor \"" + address_.processor() + "\"" );
    }
    
    // get default port if needed
    if (address_.port()=="") {
        address_.set_port( processor()->default_input_port() );
    }
    
    // test if port exists
    if (!processor()->has_input_port( address_.port() ) ) {
        throw std::out_of_range( "Unknown input port \"" + address_.processor() + "." + address_.port() + "\"." );
    }
    
    // test if slot is valid and create new slot if needed
    int slot = port()->ReserveSlot( address_.slot() );
    
    // and update slot in address
    if (slot<0) {
        throw std::out_of_range( "Unable to reserve slot \"" + std::to_string(address_.slot()) + "\" for input port \"" + address_.processor() + "." + address_.port() + "\"." );
    }
      
    address_.set_slot( slot );
    
}

IProcessor* StreamInConnector::processor() { return processor_engine_->processor(); }
IPortIn* StreamInConnector::port() { return processor()->input_port( address_.port() ); }
ISlotIn* StreamInConnector::slot() { return port()->slot( address_.slot() ); }

void StreamInConnector::Connect( StreamOutConnector* upstream ) {
    
    port()->Connect( address_.slot(), upstream );
}

bool StreamInConnector::CheckCompatibility( StreamOutConnector* upstream ) {
    
    return port()->CheckCompatibility( upstream->port() );
}

void StreamConnection::Connect(const ProcessorEngineMap& processors) {
    
    if (connected())
    { throw std::runtime_error( "Connection already made."); }
    
    // map ConnectionInfo to ConnectionPoint
    out_connector_.reset( new  StreamOutConnector(out_, processors) );
    in_connector_.reset( new  StreamInConnector(in_, processors) );
    
    // check if connectors are compatible
    if (!in_connector_->CheckCompatibility( out_connector_.get() ))
    { throw std::runtime_error("Incompatible ports."); }
    
    in_connector_->Connect( out_connector_.get() );
    
    try {
        out_connector_->Connect( in_connector_.get() );
    } catch (...) {
        // internal error
        //in_connector_->Disconnect();
        throw std::runtime_error( "Internal error: cannot connect to output slot" );
    }
    
    connected_.store(true);
}
     
