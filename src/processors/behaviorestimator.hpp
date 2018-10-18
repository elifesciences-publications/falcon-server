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
 * BehaviorEstimator: reads the likelihood computed from all sensors and
 * extracts from it an estimation of the behaviour of the animal using
 * maximum likelihood estimation.
 * 
 * input ports:
 * estimates <LikelihoodData> (1 slot)
 * 
 * output ports:
 * behavior <BehaviorData<double>> (1 slot)
 * 
 * exposed states:
 * none
 *
 * exposed methods:
 * none
 *
 * options:
 * time_bin_ms <double> - bin-size of the likelihood to be used for estimating the position
 * path_to_grid <string> - path to environment definition file
 * grid_unit <string> - cm or pixel, according to the unit used to generated the
 * imported grid
 * 
 */

#ifndef BEHAVIORESTIMATOR_HPP
#define	BEHAVIORESTIMATOR_HPP

#include "../graph/iprocessor.hpp"
#include "environment/environment.hpp"
#include "../data/likelihooddata.hpp"
#include "../data/behaviordata.hpp"

class BehaviorEstimator : public IProcessor {

public:
    virtual void Configure( const YAML::Node  & node, const GlobalContext& context) override;
    virtual void CreatePorts( ) override;
    virtual void CompleteStreamInfo( ) override;
    virtual void Prepare( GlobalContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;
    virtual void Unprepare( GlobalContext& context ) override;
    
public:
    const double DEFAULT_TIME_BIN_MS = 250.0;
    const double TOLERANCE_TIME_BIN_US = 50.0;
    const bool DEFAULT_STRICT_TIME_BIN_CHECK = true;
    
protected:
    PortIn<LikelihoodDataType>* data_in_port_;
    PortOut<BehaviorDataType>* data_out_port_;
    
    double time_bin_ms_;
    bool strict_time_bin_check_;
    size_t n_incoming_; // how many incoming likelihoods must be integrated to compute the position
    Grid* grid_;
    std::string path_to_grid_;
    std::string grid_unit_s_;
    
    LikelihoodData integrating_likelihood_;
    uint64_t counter_input_;
};

#endif	// behaviorestimator.hpp

