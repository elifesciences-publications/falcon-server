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

/* OpenEphysReader: reads raw data from a Neuralynx Digilynx data acquisition 
 * system and turns it into multiple MultiChannelData output streams 
 * based on a channel mapping
 * 
 * input ports:
 * none
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
 * address <string> - IP address of Digilynx system
 * port <unsigned int> - port of Digilynx system
 * nchannels <unsigned int> - number of channels in Digilynx system
 * batch_size <unsigned int> - how many samples to pack into single
 *   MultiChannelData bucket
 * npackets <uint64_t> - number of raw data packets to read before
 *   exiting (0 = continuous streaming)
 * update_interval <unsigned int> - time interval (in seconds) between
 *   log updates
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

#ifndef OPENEPHYSREADER_HPP
#define OPENEPHYSREADER_HPP

#include "../graph/iprocessor.hpp"
#include "nlxreader.hpp" //ChannelMap
#include "../data/multichanneldata.hpp"

#include "openephys/openephys.hpp"

#include "openephys/okFrontPanelDLL.h"
#include "openephys/rhd2000datablock.h"
#include "openephys/rhd2000evalboard.h"
#include "openephys/rhd2000registers.h"

#include <queue>
#include <map>
#include <fstream>
#include <memory>


class OpenEphysReader : public IProcessor  {
    
public:
    OpenEphysReader() : IProcessor( PRIORITY_HIGH ) {};
    
    virtual void Configure( const YAML::Node  & node, const GlobalContext& context ) override;
    virtual void CreatePorts() override;
    virtual void CompleteStreamInfo() override;
    virtual void Prepare( GlobalContext& context ) override;
    virtual void Preprocess( ProcessingContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;
    virtual void Unprepare( GlobalContext& context ) override;
      
protected:
    bool startAcquisition();
    bool stopAcquisition();
    void initialize_board( std::string fpgaconfig_filename );
    void scan_port();
    void updateRegisters( Rhd2000Registers* chipRegisters );
    void write_records( Rhd2000DataBlock origin,
        std::vector<MultiChannelData<double>*>& data_vector );
    void send_updates( bool usbDataRead );
    
public:
    static constexpr uint16_t MAX_NCHANNELS = 128;
    
protected:
    ChannelMap channelmap_;
    std::string address_;
    unsigned int port_;
    unsigned int batch_size_;
    unsigned int nchannels_;

protected:
    std::unique_ptr<Rhd2000EvalBoard> eval_board_;
    std::queue<Rhd2000DataBlock> data_queue_;
    Rhd2000Registers* chipRegisters_;
    bool deviceFound;

    uint64_t sample_counter_;
    uint64_t update_interval_;
    
    std::map<std::string, PortOut<MultiChannelDataType<double>>*> data_ports_;
    
public:
    const decltype(batch_size_) DEFAULT_BATCHSIZE = SAMPLES_PER_DATA_BLOCK;
    const decltype(nchannels_) DEFAULT_NCHANNELS = OpenEphys::NCHANNELS_PER_PORT;
    const decltype(update_interval_) DEFAULT_UPDATE_INTERVAL_SEC = 20;
  
};

#endif // openephysreader.hpp
