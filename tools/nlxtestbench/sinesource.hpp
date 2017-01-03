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

#ifndef SINESOURCE_H
#define SINESOURCE_H

#include <random>

#include "common.hpp"
#include "datasource.hpp"

class SineSource : public DataSource {
public:
    SineSource( double offset = 0.0, double amplitude = 1.0, double frequency = 1.0, double sampling_rate = 1.0, double noise_stdev = 0 );
    
    virtual std::string string();
    
    virtual bool Produce( char** data );
    
    virtual YAML::Node to_yaml() const;
    
    static SineSource* from_yaml( const YAML::Node node );
    
protected:
    double offset_;
    double amplitude_;
    double frequency_;
    double sampling_rate_;
    double noise_stdev_;
    
    uint64_t timestamp_ = 0;
    uint64_t delta_;
    
    NlxSignalRecord record_;
    
    double omega_;
    
    char buffer_[BUFFERSIZE];
    
    std::default_random_engine generator_;
    std::normal_distribution<double> distribution_;
};

#endif // SQUARESOURCE_H
