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

#include <iostream>

namespace configuration {

template <typename T>
T validate_number_option( const YAML::Node & node, std::string option, T min, T max, T default_value ) {
    
    T value = default_value;
    if (node) {
        try {
            value = node.as<T>();
            if (value<min || value>max) {
                throw ValidationError("Invalid value for configuration option " + option + ". Should be value in the range [" + std::to_string(min) + ", " + std::to_string(max) + "]." );
            }
        } catch ( YAML::BadConversion & e ) {
            throw ValidationError("Invalid value for configuration option " + option + ".");
        }
    }
    return value;
}

template <typename C>
void load_config_file( std::string filename, C & config ) {
    
    char *home = getenv("HOME");
    std::regex re ("(\\$HOME|~)");
    filename = std::regex_replace( filename, re, home );
    
    YAML::Node node;
    try { node = YAML::LoadFile( filename ); }
    catch (YAML::BadFile & e ) { // config file does not exist, save default configuration
        try {
            config.save( filename );
            std::cout << "Default configuration saved to " << filename << "." << std::endl;
        } catch ( std::exception & e ) {
            std::cout << "Warning: could not save configuration file." << std::endl;
        }
    }
    
    config.validate( node );
    
}

} // namespace configuration
