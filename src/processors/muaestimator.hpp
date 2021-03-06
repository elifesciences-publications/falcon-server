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

/*
 * MUAEstimator:
 * Computes the Multi-Unit Activity from the spike counts provided by
 * the spike detectors and outputs MUAData.
 * 
 * input ports:
 * spikes <SpikeData> (1-256 slots)
 *
 * output ports:
 * mua <MUAData> (1 slot)
 *
 * exposed states:
 * mua <double> - last measured MUA
 *
 * exposed methods:
 * none
 *
 * options:
 * bin_size <double> - default bin_size_user state
 * threshold <double> - default threshold state
 * 
 */


#ifndef MUAESTIMATOR_HPP
#define	MUAESTIMATOR_HPP

#include "../graph/iprocessor.hpp"
#include "../data/spikedata.hpp"
#include "../data/muadata.hpp"

class MUAEstimator : public IProcessor {

public:
    virtual void Configure( const YAML::Node  & node, const GlobalContext& context) override;
    virtual void CreatePorts( ) override;
    virtual void CompleteStreamInfo() override; 
    virtual void Prepare( GlobalContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;

protected:
    PortIn<SpikeDataType>* data_in_port_;;
    PortOut<MUADataType>* data_out_port_;
    
    ReadableState<double>* bin_size_;
    WritableState<double>* mua_;
    
    double initial_bin_size_;
    double current_bin_size_;
    double previous_bin_size_;
    double spike_buffer_size_;
    
    std::size_t n_spike_buffers_;
    
public:
    decltype(initial_bin_size_) DEFAULT_BIN_SIZE = 10;
    
};

#endif	// muaestimator.hpp

