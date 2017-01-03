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

#ifndef WHITENOISESOURCE_H
#define WHITENOISESOURCE_H

#include <random>

#include "common.hpp"
#include "datasource.hpp"

class WhiteNoiseSource : public DataSource {
public:
    WhiteNoiseSource( double mean = 0.0, double stdev = 1.0, double sampling_rate = 1.0 );
    
    virtual std::string string();
    
    virtual bool Produce( char** data );
    
    virtual YAML::Node to_yaml() const;
    
    static WhiteNoiseSource* from_yaml( const YAML::Node node );
    
protected:
    double mean_;
    double stdev_;
    double sampling_rate_;
    uint64_t timestamp_ = 0;
    uint64_t delta_;
    
    NlxSignalRecord record_;
    
    char buffer_[BUFFERSIZE];
    
    std::default_random_engine generator_;
    std::normal_distribution<double> distribution_;
};

#endif // WHITENOISESOURCE_H
