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
 * LikelihoodDataFileStreamer: streams LikelihoodData loaded from two NPY files.
 * One NPY file must contain the log_likelihood values that have to be streamed 
 * organized as a 2D array of size (m, n), with:
 * m : number of time bins
 * n : grid size
 * The other NPY file must contain the number of spikes related to each likelihood
 * in the previous file; the values should be organized as a 1D array of size n
 * (the numbers must be save as int32).
 * The time bin is assumed constant through the streaming and must be set with
 * a dedicated option.
 * 
 * input ports:
 * none
 * 
 * output ports:
 * data <LikelihoodData> (1 slot)
 * 
 * exposed states:
 * none
 *
 * exposed methods:
 * none
 * 
 * options:
 * path_to_likelihood <string> - full path of the NPY file containing log likelihood values
 * path_to_n_spikes <string> - full path of the NPY file containing the  number
 * of spikes for each time bin
 * time_bin_ms <double> - time bin of the likelihoods that will be streamed [ms]
 * sample_rate <double> - sample rate of the signal used for detecting the spikes
 * that generated the likelihood
 * streaming_rate <double> - (approximate) streaming rate of the each
 * generated LikelihoodData item 
 * initial_timestamp <uint64_t> - timestamp of the first streamed LikelihoodData packet
 */

#ifndef LIKELIHOODDATA_FILESTREAMER_HPP
#define	LIKELIHOODDATA_FILESTREAMER_HPP

#include "../graph/iprocessor.hpp"
#include "../data/likelihooddata.hpp"
#include "npyreader/npyreader.h"
#include "neuralynx/nlx.hpp"


class LikelihoodDataFileStreamer : public IProcessor {
    
public:
    virtual void Configure( const YAML::Node& node, const GlobalContext& context) override;
    virtual void CreatePorts() override;
    virtual void CompleteStreamInfo() override;
    virtual void Prepare( GlobalContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;
    virtual void Unprepare( GlobalContext& context ) override;  

protected:
    PortOut<LikelihoodDataType>* data_out_port_;
    
    std::string path_to_likelihood_;
    std::string path_to_n_spikes_;
    double time_bin_ms_;
    uint64_t initial_timestamp_;
    double sample_rate_;
    double streaming_rate_;
    
    FILE* fp_likelihood_;
    FILE* fp_n_spikes_;
    double** loaded_log_likelihoods_;
    int32_t* loaded_n_spikes_;
    
    uint32_t grid_size_;
    std::vector<uint64_t> generated_hw_timestamps_;
    uint32_t n_packets_to_stream_;

public:
    const uint64_t DEFAULT_INITIAL_TS = 0;
    static constexpr double DEFAULT_TIMEBIN_MS = 10.0;
    static constexpr double DEFAULT_SAMPLE_RATE = NLX_SIGNAL_SAMPLING_FREQUENCY;
    static constexpr double DEFAULT_STREAMING_RATE =
        NLX_SIGNAL_SAMPLING_FREQUENCY / (1000 * DEFAULT_TIMEBIN_MS);
};

#endif	// likelihooddatafilestreamer.hpp

