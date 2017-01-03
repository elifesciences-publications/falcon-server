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

#include "muadata.hpp"

#include "utilities/string.hpp"


void MUAData::Initialize( double bin_size) {
    
    bin_size_ = bin_size;
}

void MUAData::ClearData() {
    
    bin_size_ = 0;
    n_spikes_ = 0;
}

void MUAData::set_n_spikes( unsigned int n_spikes ) {
    
    n_spikes_ = n_spikes;
}

double MUAData::mua() const {
    
    return n_spikes_ / bin_size_ * 1e3;
}

void MUAData::set_bin_size( double bin_size ) {
    
    bin_size_ = bin_size;
}

double MUAData::bin_size() {
    
    return bin_size_;
}

unsigned int MUAData::n_spikes() {
    
    return n_spikes_;
}

void MUAData::SerializeBinary( std::ostream& stream, Serialization::Format format ) const {
    
    IData::SerializeBinary( stream, format );
    if ( format==Serialization::Format::FULL || format==Serialization::Format::COMPACT ) {       
        auto _mua = mua();
        stream.write( reinterpret_cast<const char*>( &_mua ), sizeof(decltype(_mua)) );
    }
    if ( format==Serialization::Format::FULL ) {
        stream.write( reinterpret_cast<const char*>( &n_spikes_ ), sizeof(decltype(n_spikes_)) );
        stream.write( reinterpret_cast<const char*>( &bin_size_ ), sizeof(decltype(bin_size_)) );
    }
}

void MUAData::SerializeYAML( YAML::Node & node, Serialization::Format format ) const {
    
    IData::SerializeYAML( node, format );
    if ( format==Serialization::Format::FULL || format==Serialization::Format::COMPACT ) {
        node["MUA"] = mua();
    }
    if ( format==Serialization::Format::FULL ) {
        node["n_spikes"] = n_spikes_;
        node["bin_size"] = bin_size_;
    }
}

void MUAData::YAMLDescription( YAML::Node & node, Serialization::Format format ) const {
    
    IData::YAMLDescription( node, format );
    
    if ( format==Serialization::Format::FULL || format==Serialization::Format::COMPACT ) {
        node.push_back( "MUA " + get_type_string<double>() + " (1)" );
    }
    if ( format==Serialization::Format::FULL ) {
        node.push_back( "n_spikes " + get_type_string<double>() + " (1)" );
        node.push_back( "bin_size " + get_type_string<double>() + " (1)" );
    }
}

double MUADataType::bin_size() const {
    
    return bin_size_;
}

void MUADataType::InitializeData( MUAData& item ) const {
    
    item.Initialize( bin_size_ );
}

void MUADataType::Finalize( double bin_size ) {
    
    bin_size_ = bin_size;
}
