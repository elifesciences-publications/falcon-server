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

#include "videotrackdata.hpp"

#include "utilities/string.hpp"

void VideoTrackData::Initialize( double sample_rate, std::array<int, 2> resolution  ) {
    
    sample_rate_ = sample_rate;
    resolution_ = resolution;
}

void VideoTrackData::ClearData() {

    std::fill( vt_measures_.begin(), vt_measures_.end(), 0);
    occlusion_ = false;
}

void VideoTrackData::set_x( std::int32_t x ) {
    
    vt_measures_[0] = x;
}

void VideoTrackData::set_y( std::int32_t y ) {
    
    vt_measures_[1] = y;
}

void VideoTrackData::set_angle( std::int32_t angle ) {
    
    vt_measures_[2] = angle;
}

std::int32_t VideoTrackData::x() const {
    
    return vt_measures_[0];
}

std::int32_t VideoTrackData::y() const {
    
    return vt_measures_[1];
}

void VideoTrackData::mark_as_occluded() {
    
    occlusion_ = true;
}

std::int32_t VideoTrackData::angle() const {
    
    return vt_measures_[2];
}

bool VideoTrackData::is_occluded() const {
    
    return occlusion_;
}

double VideoTrackData::sample_rate() const {
    
    return sample_rate_;
}
    
std::array<std::int32_t, 2> VideoTrackData::resolution() const {
    
    return resolution_;
}

void VideoTrackData::SerializeBinary( std::ostream& stream, Serialization::Format format ) const {
    
    IData::SerializeBinary( stream, format );
    if ( format==Serialization::Format::FULL || format==Serialization::Format::COMPACT ) {
        stream.write( reinterpret_cast<const char*>( vt_measures_.cbegin() ),
            vt_measures_.size() * sizeof(std::int32_t) );
        stream.write( reinterpret_cast<const char*>( &hardware_timestamp_ ),
                sizeof(hardware_timestamp_) );
    }
    
    if ( format==Serialization::Format::FULL ) {
        stream.write( reinterpret_cast<const char*>( &occlusion_ ),
            sizeof(decltype(occlusion_)) );
        stream.write(reinterpret_cast<const char*>( &sample_rate_ ),
            sizeof(decltype(sample_rate_)) );
        stream.write( reinterpret_cast<const char*>( resolution_.cbegin() ),
            resolution_.size() * sizeof(std::int32_t) );
    }
}

void VideoTrackData::SerializeYAML( YAML::Node & node, Serialization::Format format ) const {
    
    IData::SerializeYAML( node, format );
    if ( format==Serialization::Format::FULL || format==Serialization::Format::COMPACT ) {
        node["xy"] = std::vector<std::int32_t> { x(), y() };
        node["angle"] = angle();
        node["timestamp"] = hardware_timestamp_;
    }
    
    if ( format==Serialization::Format::FULL ) {
        node["occlusion"] = occlusion_;
        node["sample_rate"] = sample_rate_;
        node["resolution"] = std::vector<std::int32_t> { resolution_[0], resolution_[1] };
    }
}

void VideoTrackData::YAMLDescription( YAML::Node & node, Serialization::Format format ) const {
    
    IData::YAMLDescription( node, format );
    if ( format==Serialization::Format::FULL || format==Serialization::Format::COMPACT ) {
        node.push_back( "xy " + get_type_string<int32_t>() + " (2)" );
        node.push_back( "angle " + get_type_string<int32_t>() + " (1)" );
        node.push_back( "timestamp " + get_type_string<uint64_t>() + " (1)" );
    }
    
    if ( format==Serialization::Format::FULL ) {
        node.push_back( "occlusion " + get_type_string<decltype(occlusion_)>()
            + " (1)" );
        node.push_back( "sample_rate " + get_type_string<decltype(sample_rate_)>()
            + " (1)" );
        node.push_back( "resolution " + get_type_string<std::int32_t>()
            + " (2)" );
    }
}
    
void VideoTrackDataType::InitializeData( VideoTrackData& item ) const {
    
    item.Initialize( sample_rate_, resolution_ );
}

bool VideoTrackDataType::CheckCompatibility( const VideoTrackDataType& upstream ) const {
    
    return ( upstream.resolution() == resolution() ); 
}

double VideoTrackDataType::sample_rate() const {
    
    return sample_rate_;
}
    
std::array<std::int32_t, 2> VideoTrackDataType::resolution() const {
    
    return resolution_;
}