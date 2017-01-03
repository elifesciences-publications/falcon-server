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

#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <algorithm>
#include <cctype>
#include <string>
#include <ostream>

#include "yaml-cpp/yaml.h"

// forward declaration
class IData;

namespace Serialization {

static const uint8_t VERSION = 1;

enum class Format { NONE=-1, FULL, COMPACT, HEADERONLY, STREAMHEADER };
// NONE: no packet header, no data header, no data
// FULL: packet header, data header and data
// HEADERONLY: packet header, data header, no data
// STREAMHEADER: packet header, no data header, no data
// COMPACT: data only

std::string format_to_string( Format fmt );

Format string_to_format( std::string s );

class Serializer {

public:
    Serializer( Format fmt = Format::FULL, std::string description="", std::string extension="" )
        : format_(fmt), description_(description), extension_(extension) {};
    
    virtual bool Serialize( std::ostream & stream, IData* data, uint16_t streamid, uint64_t packetid ) const = 0;
    
    Format format() const;
    void set_format( Format fmt );
    
    YAML::Node DataDescription( const IData* data ) const;
    
    std::string description() const;
    std::string extension() const;
    
protected:
    Format format_;
    std::string description_;
    std::string extension_;
};

class BinarySerializer : public Serializer {

public:
    BinarySerializer( Format fmt = Format::FULL )
        : Serializer( fmt, "Compact binary format", "bin" ) {}
    bool Serialize( std::ostream & stream, IData* data, uint16_t streamid, uint64_t packetid ) const;
};

class YAMLSerializer : public Serializer {

public:
    YAMLSerializer( Format fmt = Format::FULL )
        : Serializer( fmt, "Human readable YAML format", "yaml" ) {}
    
    bool Serialize( std::ostream & stream, IData* data, uint16_t streamid, uint64_t packetid ) const;
};


Serializer* serializer_from_string( std::string s, Serialization::Format fmt = Format::FULL );

} // namespace Serialization 

#endif //serialize.hpp
