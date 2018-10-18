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

/* Dispatcher: reads raw data from a Neuralynx Digilynx data acquisition 
 * system and turns it into multiple MultiChannelData output streams 
 * based on a channel mapping
 * 
 * input ports:
 * data <MultiChannelData> (1 slot)
 *
 * output ports:
 * [configurable] <MultiChannelData> (1 slot)
 *
 * exposed states:
 * none
 *
 * exposed methods:
 * none
 *
 * options:
 * batch_size <unsigned int> - how many samples to pack into single
 *   MultiChannelData bucket
 * channelmap - mapping between AD channels and output ports
 * hardware_trigger <bool> - enable use of hardware triggered dispatching
 * hardware_trigger_channel <uint8> - which DIO channel to use as trigger
 * 
 * extra information:
 * The channelmap defines the output port names and for each port lists 
 * the AD channels that will be copied to the MultiChannelData buckets 
 * on that port. The channelmap option should be specified as follows:
 * 
 * channelmap:
 *   portnameA: [0,1,2,3,4]
 *   portnameB: [5,6]
 *   portnameC: [0,5]
 * 
 */

#ifndef DISPATCHER_HPP
#define DISPATCHER_HPP

#include <map>
#include <vector>
 
#include "../graph/iprocessor.hpp"
#include "../data/multichanneldata.hpp"


typedef std::map<std::string,std::vector<unsigned int>> ChannelMap;


class Dispatcher : public IProcessor {
public:
    Dispatcher() : IProcessor( PRIORITY_MEDIUM ) {};
    
    virtual void Configure( const YAML::Node  & node, const GlobalContext& context ) override;
    virtual void CreatePorts() override;
    virtual void CompleteStreamInfo() override;
    virtual void Prepare( GlobalContext& context ) override;
    virtual void Preprocess( ProcessingContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;
    
protected:
    PortIn<MultiChannelDataType<double>>* input_port_;
    std::map<std::string, PortOut<MultiChannelDataType<double>>*> data_ports_;
    
    ChannelMap channelmap_;
//    unsigned int n_samples_;
    unsigned int incoming_batch_size_;
    unsigned int max_n_channels_;
    
//public:
//    const decltype(batch_size_) DEFAULT_BATCHSIZE = 1;
    
  
};

#endif // dispatcher.hpp
