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

/* Rebuffer: rebuffers and downsamples multiple MultiChannelData streams.
 * No anti-aliasing filter is applied before downsampling.
 * 
 * input ports:
 * data <MultiChannelData> (1-256 slots)
 *
 * output ports:
 * data <MultiChannelData> (1-256 slots)
 *
 * exposed states:
 * none
 *
 * exposed methods:
 * none
 *
 * options:
 * buffer_size : output buffer size in samples or time
 * buffer_unit : samples or seconds
 * downsample : downsample factor
 * 
 */

#ifndef REBUFFER_HPP
#define REBUFFER_HPP

#include "../graph/iprocessor.hpp"
#include "../data/multichanneldata.hpp"

#include "neuralynx/nlx.hpp"
#include "utilities/time.hpp"

class Rebuffer : public IProcessor
{
public:
    virtual void Configure( const YAML::Node  & node, const GlobalContext& context) override;
    virtual void CreatePorts( ) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void CompleteStreamInfo( ) override;

protected:   
    PortIn<MultiChannelDataType<double>>* data_in_port_;
    PortOut<MultiChannelDataType<double>>* data_out_port_;
    
    std::string buffer_unit_;
    unsigned int buffer_size_samples_;
    double buffer_size_seconds_;
    unsigned int downsample_factor_;
    
    std::vector<decltype(buffer_size_samples_)> buffer_size_;
    
public:
    static const decltype(downsample_factor_) DEFAULT_DOWNSAMPLE_FACTOR = 1;
    const std::string DEFAULT_BUFFER_UNIT = "samples";
    static constexpr decltype(buffer_size_samples_) DEFAULT_BUFFER_SIZE_SAMPLES = 10;
    static constexpr decltype(buffer_size_seconds_) DEFAULT_BUFFER_SIZE_SECONDS =
        samples2time<decltype(buffer_size_seconds_)>(
            DEFAULT_BUFFER_SIZE_SAMPLES,
            NLX_SIGNAL_SAMPLING_FREQUENCY / DEFAULT_DOWNSAMPLE_FACTOR );
    
};

#endif //rebuffer.hpp
