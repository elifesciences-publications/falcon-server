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

/* MultiChannelFilter: filters multiple MultiChannelData streams
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
 * filter - YAML filter definition or name of file that contains filter
 * 
 * extra information:
 * Here are some example filter configurations:
 * 
 * filter:
 *   file: filters://butter_lpf_0.1fs.filter
 * 
 * filter:
 *   type: biquad
 *   gain: 3.405376527201278e-04
 *   coefficients:
 *     - [1.0, 2.0, 1.0, 1.0, -1.032069405319709, 0.275707942472944]
 *     - [1.0, 2.0, 1.0, 1.0, -1.142980502539903, 0.412801598096190]
 *     - [1.0, 2.0, 1.0, 1.0, -1.404384890471583, 0.735915191196473]
 *   description: 6th order butterworth low pass filter with cutoff at 0.1 times the sampling frequency
 * 
 * filter:
 *   type: fir
 *   description: 101 taps low pass filter with cutoff at 0.1 times the sampling frequency
 *   coefficients: [-6.24626469088e-19, -0.000309386982441, -0.000528204854007, ...]
 */

#ifndef MULTICHANNELFILTER_HPP
#define MULTICHANNELFILTER_HPP

#include "../graph/iprocessor.hpp"
#include "../data/multichanneldata.hpp"

#include <memory>
#include <dsp/filter.hpp>


class MultiChannelFilter : public IProcessor
{
public:
    virtual void Configure( const YAML::Node  & node, const GlobalContext& context) override;
    virtual void CreatePorts( ) override;
    virtual void Prepare( GlobalContext& context ) override;
    virtual void Unprepare( GlobalContext& context ) override;
    virtual void Preprocess( ProcessingContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;
    virtual void CompleteStreamInfo( ) override;

protected:
    std::unique_ptr<dsp::filter::IFilter> filter_template_;
    std::vector<std::unique_ptr<dsp::filter::IFilter>> filters_;
    
    PortIn<MultiChannelDataType<double>>* data_in_port_;
    PortOut<MultiChannelDataType<double>>* data_out_port_;
};

#endif // multichannelfilter.hpp
