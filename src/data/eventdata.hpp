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

#ifndef EVENTDATA_HPP
#define EVENTDATA_HPP

#include "idata.hpp"

typedef unsigned int EventIDType;

const std::string DEFAULT_EVENT = "none";

class EventData : public IData {

public:
    
    EventData( std::string event = DEFAULT_EVENT );
    
    void Initialize( std::string event = DEFAULT_EVENT );
    
    virtual void ClearData() override;
    
    std::string event() const;
    size_t hash() const;
    
    void set_event( std::string event );
    void set_event( const EventData &source );
    
    friend bool operator==(EventData &e1, EventData &e2);
    friend bool operator!=(EventData &e1, EventData &e2);
    
    virtual void SerializeBinary( std::ostream& stream,
        Serialization::Format format = Serialization::Format::FULL ) const override;
    virtual void SerializeYAML( YAML::Node & node,
        Serialization::Format format = Serialization::Format::FULL ) const  override;
    virtual void YAMLDescription( YAML::Node & node,
        Serialization::Format format = Serialization::Format::FULL ) const override;
    
protected:
    std::string event_;
    size_t hash_;
    
    static const unsigned int EVENT_STRING_LENGTH = 128;
};


class EventDataType : public AnyDataType {

ASSOCIATED_DATACLASS(EventData)

public:
    EventDataType( std::string default_event = DEFAULT_EVENT ) :
        AnyDataType(true), default_event_( default_event ) { }
    
    std::string default_event() const;
    
    virtual void InitializeData( EventData& item ) const;
    
    virtual std::string name() const { return DEFAULT_EVENT; }
        
protected:
    std::string default_event_;
};

#endif // eventdata.hpp
