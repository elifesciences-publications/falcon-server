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

#ifndef BEHAVIORDATA_HPP
#define	BEHAVIORDATA_HPP

#include "idata.hpp"
#include "dsp/behavior_algorithms.hpp"

#include <array>

enum BehaviorUnit {
    PIXEL,
    CM
};

const std::string PIXEL_S = "pixel";
const std::string CM_S = "cm";
const std::size_t UNIT_S_SIZE = 5;
const std::string BEHAVIORDATA_S = "behavior";

class BehaviorData : public IData {

public:
    
    void Initialize( BehaviorUnit unit );
    
    virtual void ClearData() override;

    std::string unit_string() const;
    
    void set_linear_position( double linear_position );
    double linear_position() const;
    
    void set_speed( double speed );
    void set_speed_sign( dsp::behavior_algorithms::SpeedSign sign );
    double speed() const;
    double signed_speed() const;
    
    void set_head_direction( bool head_direction_ );
    bool head_direction() const;
    
    void convert_to_pixel( double cm_to_pixel );
    void convert_to_cm( double pixel_to_cm );
    
    virtual void SerializeBinary( std::ostream& stream,
        Serialization::Format format = Serialization::Format::FULL ) const override;
    virtual void SerializeYAML( YAML::Node & node,
        Serialization::Format format = Serialization::Format::FULL ) const  override;
    virtual void YAMLDescription( YAML::Node & node,
        Serialization::Format format = Serialization::Format::FULL ) const override;
    
protected:
    BehaviorUnit unit_;
    double linear_position_;
    double speed_;
    dsp::behavior_algorithms::SpeedSign speed_sign_;
    
protected:
    std::string POSITION_S = "linear_position";
    std::string SPEED_S = "speed";
    std::string UNIT_S = "unit";
    std::string SPEED_SIGN_S = "speed_sign";
};


class BehaviorDataType : public AnyDataType {

ASSOCIATED_DATACLASS(BehaviorData)

public:
    BehaviorDataType() : AnyDataType(true) {}
    
    BehaviorUnit unit() const;
    
    double sample_rate() const;
    
    virtual void InitializeData( BehaviorData& item ) const;
    
    virtual void Finalize( BehaviorUnit unit, double sample_rate );

    virtual void Finalize( BehaviorDataType& upstream );
    
protected:
    BehaviorUnit unit_;
    double sample_rate_;
        
};

#endif	// behaviordata.hpp

