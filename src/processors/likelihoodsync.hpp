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
 * LikelihoodSync: synchronizes the likelihoods obtained by different neural decoders
 * and computes the population likelihood by sum in log-space
 * 
 * input ports:
 * data <LikelihoodData> (1-64 slots)
 *
 * output ports:
 * events <LikelihoodData> (1 slot)
 *
 * exposed states:
 * none
 *
 * exposed methods:
 * none
 *
 * options:
 * bin_size <double> - bin-size of the likelihood that have to be synced and multiplied
 * path_to_grid <string> - path to environment definition file
 * 
 */

#ifndef LIKELIHOODSYNC_HPP
#define LIKELIHOODSYNC_HPP

#include "neuralynx/nlx.hpp"
#include "../graph/iprocessor.hpp"
#include "../data/likelihooddata.hpp"


class LikelihoodSync : public IProcessor {
    
public:
    virtual void Configure(const YAML::Node& node, const GlobalContext& context) override;
    virtual void CreatePorts() override;
    virtual void CompleteStreamInfo( ) override;
    virtual void Prepare( GlobalContext& context ) override;
    virtual void Preprocess( ProcessingContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;
   
protected:
    // small method for throwing similarly structured errors
    void throw_error( std::string name, double value_s0, double current_value,
        SlotType s, std::string processor_name) const;
    
protected:
    PortIn<LikelihoodDataType>* data_in_port_;
    PortOut<LikelihoodDataType>* data_out_port_;
  
    double time_bin_ms_;
    unsigned int n_expected_likelihoods_;
    uint64_t n_likelihoods_received_;
    uint64_t global_counter_likelihoods_synced_;
    uint64_t n_synced_likelihoods_;
    
    const double NO_BIN_SIZE = -98546.678;
};

#endif // likelihoodsync.hpp
