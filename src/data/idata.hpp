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

#ifndef IDATA_HPP
#define IDATA_HPP

#include <deque>
#include <string>
#include <memory> 
#include <cassert>
#include <chrono>
#include <limits>

#include "../ringbuffer.hpp"

#include "g3log/src/g2log.hpp"
#include "utilities/time.hpp"

#include "serialize.hpp"
#include "yaml-cpp/yaml.h"

const double IRREGULARSTREAM = std::numeric_limits<double>::min();

// Factory for DATATYPE::DATACLASS items with support for post-construction initialization
template <typename DATATYPE>
class DataFactory : public IFactory<typename DATATYPE::DATACLASS> {
public:
    DataFactory( DATATYPE& datatype ) : datatype_( datatype ) {}
    
    virtual typename DATATYPE::DATACLASS* NewInstance( const int& size ) const override final {
        
        auto items = new typename DATATYPE::DATACLASS[size];
        for (int k=0; k<size; k++) {
            datatype_.InitializeData( items[k] );
        }
        return items;
    }

protected:
    DATATYPE datatype_;
};

// Checks if one data type object is compatible with another data type object
template <class AbstractType, class ConcreteType>
bool CheckDataType( const AbstractType& a, const ConcreteType& c ) {
    try {
        // casting Concrete DataType to Abstract DataType
        auto cast = static_cast<const AbstractType&>(c);
        return a.CheckCompatibility( cast );
    } catch (std::bad_cast const & e) {
        return false;
    }
}

// All concrete data types need to be associated with a specific data class
// Use this macro in class declaration
#define ASSOCIATED_DATACLASS(T) public:\
typedef T DATACLASS;\

// Base class for all data classes
class IData {
public:
    IData() : hardware_timestamp_(0), serial_number_(0) {}
    
    virtual ~IData() {}
	
    virtual void ClearData() = 0;
    
    bool eos() const;
    void set_eos( bool value=true );
    void clear_eos();
    
    void set_serial_number( uint64_t n );
    uint64_t serial_number() const;
	
    void set_source_timestamp( );
    void set_source_timestamp( TimePoint t );
    
    TimePoint source_timestamp() const;
	
    template <typename DURATION=std::chrono::microseconds>
    DURATION time_passed() const { return std::chrono::duration_cast<DURATION>( Clock::now() - source_timestamp_ ); }
	
    template <typename DURATION=std::chrono::microseconds>
    DURATION time_since( TimePoint reference ) const { return std::chrono::duration_cast<DURATION>( Clock::now() - reference ); }
	
    uint64_t hardware_timestamp() const;
    void set_hardware_timestamp( uint64_t t );
    
    void CloneTimestamps( const IData& data );
	
    virtual void SerializeBinary( std::ostream& stream, Serialization::Format format ) const;
    virtual void SerializeYAML( YAML::Node & node, Serialization::Format format ) const;
    virtual void YAMLDescription( YAML::Node & node, Serialization::Format format ) const;
    
protected:
    TimePoint source_timestamp_;
    uint64_t hardware_timestamp_; // e.g. from Neuralynx
    uint64_t serial_number_;
    bool end_of_stream_ = false;
};

// Base class for all data types
// AnyDataType is associated with the IData class
class AnyDataType {
public:

    typedef IData DATACLASS;
    
    AnyDataType( bool finalized = true ) : finalized_(finalized) {}
    
    virtual ~AnyDataType() {};
	
    virtual bool CheckCompatibility( const AnyDataType& t ) const;
    virtual void InitializeData( IData& item ) const {};
    
    bool finalized() const;
    
    virtual void Finalize();
    
    virtual std::string name() const { return "any"; }
    
protected:
    bool finalized_;
};

// Represents the data stream generated by an output port
class IStreamInfo {
public:
    
    double stream_rate() const;
    
    virtual bool finalized() const = 0;
    
    void Finalize( double stream_rate = IRREGULARSTREAM );
    
protected:
    double stream_rate_ = 1.0;
    bool finalized_ = false;
};

// Data type specific implementation of stream info
template <typename DATATYPE>
class StreamInfo : public IStreamInfo {
public:
    StreamInfo( DATATYPE& datatype ) : datatype_(datatype) {}
    
    DATATYPE& datatype() { return datatype_; }
    
    virtual bool finalized() const {
        
        return (finalized_ && datatype_.finalized());
    }
    
protected:
    DATATYPE datatype_; // our own copy of data type passed to constructor
};


#endif // idata.hpp
