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

#include "eventdata.hpp"

EventData::EventData( std::string event ) {
    
    set_event( event );
}

void EventData::Initialize( std::string event ) {
    
    set_event( event );
}

void EventData::ClearData() {

    set_event( "none" );
}

std::string EventData::event() const {
    
    return event_;
}

size_t EventData::hash() const {
    
    return hash_;
}

void EventData::set_event( std::string event ) {
    
    event_ = event;
    hash_ = std::hash<std::string>()( event_ );
}

void EventData::set_event( const EventData &source ) {
    event_ = source.event();
    hash_ = source.hash();
}

bool operator==(EventData &e1, EventData &e2) {
    
    return e1.hash_==e2.hash_;
}

bool operator!=(EventData &e1, EventData &e2) {
    
    return e1.hash_!=e2.hash_;
}

void EventData::SerializeBinary( std::ostream& stream, Serialization::Format format ) const {
    
    IData::SerializeBinary( stream, format );
    if (format==Serialization::Format::FULL || format==Serialization::Format::COMPACT) {
        std::string buffer = event_;
        buffer.resize(EVENT_STRING_LENGTH);
        stream.write( buffer.data(), EVENT_STRING_LENGTH );
    }
}

void EventData::SerializeYAML( YAML::Node & node, Serialization::Format format ) const {
    
   IData::SerializeYAML( node, format );
   if (format==Serialization::Format::FULL || format==Serialization::Format::COMPACT) {
       node["event"] = event_;
   }
}

void EventData::YAMLDescription( YAML::Node & node, Serialization::Format format ) const {
    
    IData::YAMLDescription( node, format );
    if (format==Serialization::Format::FULL || format==Serialization::Format::COMPACT) {
        node.push_back("event_string str (" + std::to_string( EVENT_STRING_LENGTH ) + ")" );
    }
}

std::string EventDataType::default_event() const {
    
    return default_event_;
}
    
void EventDataType::InitializeData( EventData& item ) const {
    
    item.Initialize( default_event_ );
}
