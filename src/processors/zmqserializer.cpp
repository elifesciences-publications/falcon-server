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

#include "zmqserializer.hpp"

#include "../data/idata.hpp"
#include "utilities/zmqutil.hpp"


void ZMQSerializer::CreatePorts() {
    
    data_port_ = create_input_port(
        "data",
        AnyDataType(),
        PortInPolicy( SlotRange(1,256), false, 0 ) );
}

void ZMQSerializer::Configure( const YAML::Node & node, const GlobalContext& context ) {
    
    // local port to stream data to
    // in case interleave == true, then it specifies the first port in 
    // a sequence of ports used for streaming
    port_ = node["port"].as<unsigned int>( DEFAULT_PORT );
    
    // binary or yaml encoding
    encoding_ = node["encoding"].as<std::string>( DEFAULT_ENCODING );
    
    // data format
    format_ = Serialization::string_to_format( node["format"].as<std::string>( Serialization::format_to_string( DEFAULT_FORMAT ) ) );
    
    // whether data streams from different input slots are interleaved
    interleave_ = node["interleave"].as<bool>( DEFAULT_INTERLEAVE );
}

void ZMQSerializer::Preprocess(ProcessingContext& context) {
    
    std::string address;
    
    sockets_.clear();
    
    if (interleave_) {
        sockets_.push_back( std::move(std::unique_ptr<zmq::socket_t>( new zmq::socket_t (context.run().global().zmq(), ZMQ_PUB) ) ) );
        address = "tcp://*:" + std::to_string(port_);
        sockets_.back()->bind ( address.c_str() );
    } else {
        for (int k=0; k<data_port_->number_of_slots(); ++k) {
            sockets_.push_back( std::move(std::unique_ptr<zmq::socket_t>( new zmq::socket_t (context.run().global().zmq(), ZMQ_PUB) ) ) );
            address = "tcp://*:" + std::to_string(port_ + k);
            sockets_.back()->bind ( address.c_str() );
        }
    }
    
    serializer_.reset( Serialization::serializer_from_string( encoding_, format_ ) );
    
    packetid_.assign( data_port_->number_of_slots(), 0 );
    
}

void ZMQSerializer::Process(ProcessingContext& context) {
      
    std::vector<IData*> data;
    
    int nslots = data_port_->number_of_slots();
    
    unsigned int idx = 0;
    std::stringstream buffer;
    
    while (!context.terminated()) {
        
        for (int k=0; k<nslots; ++k) {
            
            if (!data_port_->slot(k)->RetrieveDataAll( data )) {break;}
            
            if (!interleave_) {idx=k;}
            
            for (auto & it : data) {
                
                buffer.str("");
                buffer.clear();
                
                if (serializer_->Serialize( buffer, it, k, packetid_[k]++ )) {
                    if (!s_send( *(sockets_[idx]), buffer.str() )) {
                        LOG(DEBUG) << "failed to send zmq message.";
                    }
                } else  {
                    LOG(WARNING) << name() << ": Unable to serialize data stream " << k;
                }
            }
            
            data_port_->slot(k)->ReleaseData();
            
        }
        
    }
    
}

void ZMQSerializer::Postprocess( ProcessingContext& context ) {
    
    sockets_.clear();
    serializer_.reset();
    
    for (SlotType k=0; k<data_port_->number_of_slots(); k++) {
        LOG(UPDATE) << name() << ": stream " << k <<
            ": received and serialized over network "
            << packetid_[k] << " data packets.";
    }
}
