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

/* ZMQSerializer: serializes data streams to cloud
 * 
 * input ports:
 * data <IData> (1-256 slots)
 *
 * output ports:
 * none
 *
 * exposed states:
 * none
 *
 * exposed methods:
 * none
 *
 * options:
 * port <unsigned int> - port
 * encoding <string> - binary/yaml
 * format <string> - full/nodata/compact
 * interleaved <bool>
 * 
 */

#ifndef ZMQSERIALIZER_HPP
#define ZMQSERIALIZER_HPP

#include <zmq.hpp>
#include "yaml-cpp/yaml.h"
#include "../graph/iprocessor.hpp"
#include "../data/serialize.hpp"

class ZMQSerializer : public IProcessor
{
public:
    virtual void CreatePorts() override;
    virtual void Configure( const YAML::Node & node, const GlobalContext& context ) override;
    virtual void Preprocess( ProcessingContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;
    
protected:
    PortIn<AnyDataType>* data_port_;
    
    std::string encoding_;
    Serialization::Format format_;
    unsigned int port_;
    bool interleave_;
    
    std::vector<std::unique_ptr<zmq::socket_t>> sockets_;
    std::vector<uint64_t> packetid_;
    std::unique_ptr<Serialization::Serializer> serializer_;
    
public:
    const std::string DEFAULT_ENCODING = "binary";
    const Serialization::Format DEFAULT_FORMAT = Serialization::Format::FULL;
    const unsigned int DEFAULT_PORT = 7777;
    const bool DEFAULT_INTERLEAVE = false;
};

#endif //zmqserializer.hpp
