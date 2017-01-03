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

/* RunningStats: computes running statistics
 * 
 * input ports:
 * data <MultiChannelData> (1 slot)
 *
 * output ports:
 * data <MultiChannelData> (1 slot)
 *
 * exposed states:
 * none
 *
 * exposed methods:
 * none
 *
 * options:
 * integration_time <double> - time window for exponential smoothing
 * outlier_protection <bool> - enable outlier protectection. Outliers are
 *   values larger than a predefined z-score. The contribution of an
 *   outlier is reduced by an amount that depends on the magnitude of
 *   the outlier.
 * outlier_zscore <double> - z-score that defines an outlier
 * outlier_half_life <double> - the number of standard deviations above
 *   the outlier z-score at which the influence of the outlier is halved.
 * 
 */
 
#ifndef RUNNINGSTATS_H
#define RUNNINGSTATS_H

#include "../graph/iprocessor.hpp"

#include <memory>

#include "../data/multichanneldata.hpp"
#include "dsp/algorithms.hpp"

class RunningStats : public IProcessor
{
public:
    virtual void Configure( const YAML::Node & node, const GlobalContext& context) override;
    virtual void CreatePorts( ) override;
    virtual void CompleteStreamInfo( ) override;
    virtual void Preprocess( ProcessingContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;

protected:
  
    PortIn<MultiChannelDataType<double>>* data_in_port_;
    PortOut<MultiChannelDataType<double>>* data_out_port_;
    
    double integration_time_;
    bool outlier_protection_;
    double outlier_zscore_;
    double outlier_half_life_;
    
    std::unique_ptr<dsp::algorithms::RunningMeanMAD> stats_;

public:
    const double DEFAULT_INTEGRATION_TIME = 1.0;
    const bool DEFAULT_OUTLIER_PROTECTION = false;
    const double DEFAULT_OUTLIER_ZSCORE = 6.0;
    const double DEFAULT_OUTLIER_HALF_LIFE = 2.0;
    
};

#endif
