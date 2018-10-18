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

#ifndef VIDEOTRACK_DATA_HPP
#define VIDEOTRACK_DATA_HPP

#include <array>
#include "idata.hpp"

const std::string VIDEOTRACKDATA_S = "vt"; 

class VideoTrackData : public IData {

public:
    
    void Initialize( double sample_rate, std::array<int, 2> resolution );
    
    virtual void ClearData() override;
    
    void set_x( std::int32_t x );
    void set_y( std::int32_t y );
    void set_angle( std::int32_t angle );
    
    std::int32_t x() const;
    std::int32_t y() const;
    std::int32_t angle() const;
    
    void mark_as_occluded();
    bool is_occluded() const;
    
    double sample_rate() const;
    std::array<int, 2> resolution() const;
    
    virtual void SerializeBinary( std::ostream& stream,
        Serialization::Format format = Serialization::Format::FULL ) const override;
    virtual void SerializeYAML( YAML::Node & node,
        Serialization::Format format = Serialization::Format::FULL ) const  override;
    virtual void YAMLDescription( YAML::Node & node,
        Serialization::Format format = Serialization::Format::FULL ) const override;
    
protected:
    std::array<std::int32_t, 3> vt_measures_; // [x, y, angle]
    bool occlusion_;
    double sample_rate_;
    std::array<std::int32_t, 2> resolution_;
    
};


class VideoTrackDataType : public AnyDataType {

ASSOCIATED_DATACLASS(VideoTrackData)

public:
    VideoTrackDataType( double sample_rate, std::array<std::int32_t, 2> resolution):
        AnyDataType(false), sample_rate_(sample_rate), resolution_(resolution) { }
    
    virtual void InitializeData( VideoTrackData& item ) const;
    
    bool CheckCompatibility( const VideoTrackDataType& upstream ) const;
    
    double sample_rate() const;
    
    std::array<std::int32_t, 2> resolution() const;
    
protected:
    double sample_rate_;
    std::array<std::int32_t, 2> resolution_;
        
};

#endif // videotrackdata.hpp
