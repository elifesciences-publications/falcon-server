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

#include "serialize.hpp"
#include "../data/idata.hpp"

namespace Serialization {

std::string format_to_string( Format fmt ) {
    std::string s;
#define MATCH(p) case(Serialization::Format::p): s = #p; break;
    switch(fmt){
        MATCH(NONE)
        MATCH(FULL);
        MATCH(COMPACT);
        MATCH(HEADERONLY);
        MATCH(STREAMHEADER);
    }
#undef MATCH
    return s;
}

Format string_to_format( std::string s ) {
    
    std::transform(s.begin(), s.end(), s.begin(), (int (*)(int))std::toupper);
#define MATCH(p) if (s==#p) { return Serialization::Format::p;}
    MATCH(NONE)
    MATCH(FULL);
    MATCH(COMPACT);
    MATCH(HEADERONLY);
    MATCH(STREAMHEADER);
    throw std::runtime_error("Invalid Serialization::Format value.");
#undef MATCH
}

Format Serializer::format() const {

    return format_;
}

void Serializer::set_format( Format fmt ) {
    
    format_ = fmt;
}

std::string Serializer::description() const {
    
    return description_;
}

std::string Serializer::extension() const {
    
    return extension_;
}

YAML::Node Serialization::Serializer::DataDescription( const IData* data ) const {
    
    YAML::Node node;
    
    if ( format_==Serialization::Format::FULL || 
         format_==Serialization::Format::HEADERONLY ||
         format_==Serialization::Format::STREAMHEADER ) {
        node.push_back( "stream uint16 (1)" );
        node.push_back( "packet uint64 (1)" );
    }
    
    data->YAMLDescription( node, format_ );
    
    return node;
}


bool Serialization::BinarySerializer::Serialize( std::ostream & stream, IData* data, uint16_t streamid, uint64_t packetid ) const {
    
    if (format_ == Serialization::Format::NONE) { return true; }
    
    if (format_ == Serialization::Format::COMPACT) {
        data->SerializeBinary( stream, format_ );
    } else {
        stream.write( reinterpret_cast<const char*>(&streamid), sizeof(streamid) );
        stream.write( reinterpret_cast<const char*>(&packetid), sizeof(packetid) );
        data->SerializeBinary( stream, format_ );
    }
    
    return true;
}


bool Serialization::YAMLSerializer::Serialize( std::ostream & stream, IData* data, uint16_t streamid, uint64_t packetid ) const {
    
    if (format_ == Serialization::Format::NONE) { return true; }
    
    YAML::Emitter emit( stream );
    YAML::Node node;
    
    if (format_ == Serialization::Format::COMPACT) {
        data->SerializeYAML( node, format_ );
        emit << YAML::BeginSeq;
        emit << YAML::Flow << node;
        emit << YAML::EndSeq;
        stream << std::endl;
    } else {
        emit << YAML::BeginSeq << YAML::BeginMap;
        emit << YAML::Key << "stream" << YAML::Value << streamid;
        emit << YAML::Key << "packet" << YAML::Value << packetid;
        if (format_ != Serialization::Format::STREAMHEADER) {
            data->SerializeYAML( node, format_ );
            emit << YAML::Key << "data" << YAML::Value << node;
        }
        emit << YAML::EndMap << YAML::EndSeq;
        stream << std::endl;
    }
    
    return true;
}


Serializer* serializer_from_string( std::string s, Serialization::Format fmt ) {
    
    if (s=="binary") { return new Serialization::BinarySerializer( fmt); }
    if (s=="yaml") { return new Serialization::YAMLSerializer( fmt); }
    
    throw std::runtime_error("Unknown serializer.");

}

} // namespace Serialization
