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


#include "sinesource.hpp"
#include "utilities/string.hpp"
#include <stdexcept>
#include <cmath>

SineSource::SineSource( double offset, double amplitude, double frequency, double sampling_rate, double noise_stdev ) :
    
    offset_(offset), amplitude_(amplitude), frequency_(frequency), sampling_rate_(sampling_rate), noise_stdev_(noise_stdev), delta_(1000000/sampling_rate), distribution_(0.0, noise_stdev) {
    
    omega_ = 2 * 3.14159265358979 * frequency_ / 1000000;
    
}
    
std::string SineSource::string() {
    return "sine wave (fs = " + to_string_n(sampling_rate_) + " Hz, " +
           "offset = " + to_string_n(offset_) + " uV, " +
           "amplitude = " + to_string_n(amplitude_) + " uV, " +
           "frequency = " + to_string_n(frequency_) + " Hz, "
           "noise stdev = " + to_string_n(noise_stdev_) + " uV)";
}
    
bool SineSource::Produce( char** data ) {
        
    record_.set_data( distribution_(generator_) + offset_ + amplitude_*std::sin( timestamp_*omega_ ) );
    record_.set_timestamp( timestamp_ );
    timestamp_ = timestamp_ + delta_;
    record_.ToNetworkBuffer( buffer_, BUFFERSIZE );
    *data = buffer_;
    return true;
}

YAML::Node SineSource::to_yaml() const {
    
    YAML::Node node;
    
    node["offset"] = offset_;
    node["amplitude"] = amplitude_;
    node["frequency"] = frequency_;
    node["sampling_rate"] = sampling_rate_;
    node["noise_stdev"] = noise_stdev_;
    
    return node;
}

SineSource* SineSource::from_yaml( const YAML::Node node ) {
    
    return new SineSource( node["offset"].as<double>(0.0), 
                           node["amplitude"].as<double>(1.0),
                           node["frequency"].as<double>(1.0),
                           node["sampling_rate"].as<double>(32000),
                           node["noise_stdev"].as<double>(0) );
}