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

#include "configuration.hpp"

#include <stdlib.h>
#include <sys/stat.h>
#include <fstream>


bool configuration::validate_bool_option( const YAML::Node & node, std::string option, bool default_value ) {
    
    bool value = default_value;
    if (node) {
        try { value = node.as<bool>(); }
        catch ( YAML::TypedBadConversion<bool> & e ) {
            throw ValidationError( "Invalid value for configuration option " + option + ". Should be either true or false." );
        }
    }
    return value;
}

std::string configuration::validate_path_option( std::string filename, std::string option, bool create ) {
    
    char *home = getenv("HOME");
    std::regex re ("(\\$HOME|~)");
    
    // load configuration file
    std::string value = std::regex_replace( filename, re, home );
    char *abs_path = realpath( value.c_str(), NULL);
    
    if (abs_path == NULL) {
        if (!create) {
            throw ValidationError( "Invalid value for configuration option " + option + ". Should be an existing path." );
        } else {
            
            if ( mkdir( value.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH )!=0 ) {
                if (errno != EEXIST) {
                    throw ValidationError( "Error processing configuration option " + option + ". Cannot create path." );
                }
            }
        }
    } else {
        value = abs_path;
        free(abs_path);
    }
    
    return value;
}

std::string configuration::validate_path_option( const YAML::Node & node, std::string option, bool create, std::string default_value ) {
    
    std::string value = default_value;
    if (node) { value = node.as<std::string>(); }
    return validate_path_option( value, option, create );
}

void configuration::Configuration::save( std::string config_file ) const {
    
    YAML::Node config;
    to_yaml( config );
    
    YAML::Emitter yaml_emitter;
    yaml_emitter << config;

    std::ofstream out(config_file);
    out << yaml_emitter.c_str();
}
    
void configuration::Configuration::load( std::string config_file ) {
    
    YAML::Node node = YAML::LoadFile( config_file );
    validate( node );
}

void configuration::Configuration::validate () {
    
    YAML::Node node;
    from_yaml( node );
}

void configuration::Configuration::validate ( const YAML::Node & node ) {
    
    from_yaml( node );
}

