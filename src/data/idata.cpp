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

#include "idata.hpp"

bool IData::eos() const {
    
    return end_of_stream_;
}

void IData::set_eos( bool value ) {
    
    end_of_stream_ = value;
}

void IData::clear_eos() {
    
    end_of_stream_ = false;
}

void IData::set_serial_number( uint64_t n ) {
    
    serial_number_ = n;
}

uint64_t IData::serial_number() const {
    
    return serial_number_;
}

void IData::set_source_timestamp( ) {
    
    source_timestamp_ = Clock::now();
}

void IData::set_source_timestamp( TimePoint t ) {
    
    source_timestamp_ = t;
}

TimePoint IData::source_timestamp() const {
    
    return source_timestamp_;
}

uint64_t IData::hardware_timestamp() const {
    
    return hardware_timestamp_;
}

void IData::set_hardware_timestamp( uint64_t t ) {
    
    hardware_timestamp_ = t;
}

void IData::CloneTimestamps( const IData& data ) {

    source_timestamp_ = data.source_timestamp_;
    hardware_timestamp_ = data.hardware_timestamp_;
}
	
void IData::SerializeBinary( std::ostream& stream, Serialization::Format format ) const {
    
    if (format == Serialization::Format::FULL || format == Serialization::Format::HEADERONLY) {
        uint64_t t = std::chrono::duration_cast<std::chrono::microseconds>( source_timestamp_.time_since_epoch()).count();
        stream.write( reinterpret_cast<const char*>(&t), sizeof(t) );
        stream.write( reinterpret_cast<const char*>(&hardware_timestamp_), sizeof(hardware_timestamp_) );
        stream.write( reinterpret_cast<const char*>(&serial_number_), sizeof(serial_number_) );
    }
}

void IData::SerializeYAML( YAML::Node & node, Serialization::Format format ) const {
    
    // FULL, HEADERONLY : add timestamps
    // otherwise: do nothing
    if (format == Serialization::Format::FULL || format == Serialization::Format::HEADERONLY) {
        node["source_ts"] = static_cast<uint64_t>( std::chrono::duration_cast<std::chrono::microseconds>( source_timestamp_.time_since_epoch()).count() );
        node["hardware_ts"] = hardware_timestamp_;
        node["serial_number"] = serial_number_;
    }
}

void IData::YAMLDescription( YAML::Node & node, Serialization::Format format ) const {
    
    // FULL, HEADERONLY : add timestamps
    // otherwise: do nothing
    if (format == Serialization::Format::FULL || format == Serialization::Format::HEADERONLY) {
        node.push_back("source_ts uint64 (1)");
        node.push_back("hardware_ts uint64 (1)");
        node.push_back("serial_number uint64 (1)");
    }
}

bool AnyDataType::CheckCompatibility( const AnyDataType& t ) const {
    
    return true;
}

bool AnyDataType::finalized() const {
    
    return finalized_;
}

void AnyDataType::Finalize() {
    
    finalized_ = true;
}

double IStreamInfo::stream_rate() const {
    
    return stream_rate_;
}
    
void IStreamInfo::Finalize( double stream_rate ) {
    
    stream_rate_ = stream_rate;
    finalized_ = true;
}
