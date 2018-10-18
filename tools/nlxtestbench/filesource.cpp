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


#include "filesource.hpp"
#include "utilities/string.hpp"

#include <iostream>
#include <stdexcept>

FileSource::FileSource( std::string file, bool cycle ) : file_(file), cycle_(cycle) {
    
    try {
        raw_data_file.open( file_, std::ios::in | std::ios::binary );
        raw_data_file.seekg( std::ios::beg );
    } catch (...) {
        throw std::runtime_error( "Unable to open file " + file_ + "." );
    }
    
    if (raw_data_file.bad() | raw_data_file.fail()) {
        throw std::runtime_error( "Unable to open file " + file_ + 
        ". Check if filepath is correct.\n" );
    }
}
    
FileSource::~FileSource() {
    
    raw_data_file.close();
}
    
std::string FileSource::string() {
    
    return "file \"" + file() + "\" (fs = " + 
        to_string_n(NLX_SIGNAL_SAMPLING_FREQUENCY) + ")";
}

std::string FileSource::file() const {
    
    return file_;
}

bool FileSource::Produce( char** data ) {
        
    raw_data_file.read( buffer_, BUFFERSIZE );
    
    if (raw_data_file.eof()) {
        if (cycle_) {
            raw_data_file.clear();
            raw_data_file.seekg( std::ios::beg );
            raw_data_file.read( buffer_, BUFFERSIZE );
        }
        else { return false; }
    }
        
    if (!raw_data_file) {
        std::cout << "Error reading from data file." << raw_data_file.bad()
                  << " " << raw_data_file.fail() << " " << raw_data_file.eof() << std::endl;
        return false;
    }
    
    *data = buffer_;
    
    return true;
}

YAML::Node FileSource::to_yaml() const {
    
    YAML::Node node;
    
    node["file"] = file_;
    node["cycle"] = cycle_;
    
    return node;
    
}

FileSource* FileSource::from_yaml( const YAML::Node node ) {
        
    return new FileSource( node["file"].as<std::string>(), node["cycle"].as<bool>(false) );
}