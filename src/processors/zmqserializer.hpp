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
