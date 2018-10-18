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

#include "behaviordata.hpp"
#include "utilities/string.hpp"

void BehaviorData::Initialize( BehaviorUnit unit ) {
    
    unit_ = unit;
}

void BehaviorData::ClearData() {
    
    linear_position_ = 0;
    speed_ = 0;
}

std::string BehaviorData::unit_string() const {
    
    if ( unit_ == BehaviorUnit::CM ) {return CM_S;}
    if ( unit_ == BehaviorUnit::PIXEL ) {return PIXEL_S;}
    return "none";
}

double BehaviorData::linear_position() const {
    
    return linear_position_;
}

double BehaviorData::speed() const {
    
    return speed_;    
}

double BehaviorData::signed_speed() const {
    
    if ( speed_sign_ == dsp::behavior_algorithms::SpeedSign::POSITIVE ) {return speed_;}
    return -speed_;
}

void BehaviorData::set_linear_position( double linear_position ) {
    
    linear_position_ = linear_position;
}

void BehaviorData::set_speed( double speed ) {
    
    speed_ = speed;
}

void BehaviorData::set_speed_sign( dsp::behavior_algorithms::SpeedSign sign ) {
    
    speed_sign_ = sign;
}

void BehaviorData::convert_to_pixel( double cm_to_pixel ) {
    
    if ( unit_ == BehaviorUnit::CM ) {
        linear_position_ /= cm_to_pixel; 
        speed_ /= cm_to_pixel;
        unit_ = BehaviorUnit::PIXEL;
    }
}

void BehaviorData::convert_to_cm( double pixel_to_cm ) {
    
    if ( unit_ == BehaviorUnit::PIXEL ) {
        linear_position_ /= pixel_to_cm; 
        speed_ /= pixel_to_cm;
        unit_ = BehaviorUnit::CM;
    }
}

void BehaviorData::SerializeBinary( std::ostream& stream,
    Serialization::Format format ) const {
    
    IData::SerializeBinary( stream, format );
    if ( format==Serialization::Format::FULL || format==Serialization::Format::COMPACT ) {
        
        stream.write( reinterpret_cast<const char*>( &linear_position_ ),
            sizeof(decltype(linear_position_)) );
        stream.write( reinterpret_cast<const char*>( &speed_ ),
            sizeof(decltype(speed_)) );
        std::string buffer = unit_string();
        buffer.resize( UNIT_S_SIZE );
        stream.write( buffer.data(), UNIT_S_SIZE );
        
        bool speed_sign_b = false;
        if ( speed_sign_ == dsp::behavior_algorithms::SpeedSign::POSITIVE ) {
            speed_sign_b = true;
        }
        stream.write( reinterpret_cast<const char*>( &speed_sign_b ),
            sizeof(speed_sign_b) );
    }
}

void BehaviorData::SerializeYAML( YAML::Node & node,
    Serialization::Format format ) const {
    
    IData::SerializeYAML( node, format );
    if ( format==Serialization::Format::FULL || format==Serialization::Format::COMPACT ) {
        node[POSITION_S] = linear_position_;
        node[SPEED_S] = speed_;
        node[UNIT_S] = unit_string();
        if ( speed_sign_ == dsp::behavior_algorithms::SpeedSign::POSITIVE ) {
            node[SPEED_SIGN_S] = true;
        } else {
            node[SPEED_SIGN_S] = false;
        }
    }
}

void BehaviorData::YAMLDescription( YAML::Node & node, Serialization::Format format ) const {
    
    IData::YAMLDescription( node, format );
    if ( format==Serialization::Format::FULL || format==Serialization::Format::COMPACT ) {
        node.push_back( POSITION_S + " " + get_type_string<double>() + " (1)" );
        node.push_back( SPEED_S + " " + get_type_string<double>() + " (1)" );
        node.push_back( UNIT_S + " string (" + std::to_string( UNIT_S_SIZE ) + ")" );
        node.push_back( SPEED_SIGN_S + " " + get_type_string<bool>() + " (1)" );
    }
}

void BehaviorDataType::InitializeData( BehaviorData& item ) const {
    
    item.Initialize( unit_ );
}

BehaviorUnit BehaviorDataType::unit() const {
    
    return unit_;
}

double BehaviorDataType::sample_rate() const {
    
    return sample_rate_;
}

void BehaviorDataType::Finalize( BehaviorUnit unit, double sample_rate ) {
    
    unit_ = unit;
    sample_rate_ = sample_rate;
    AnyDataType::Finalize();
}

void BehaviorDataType::Finalize( BehaviorDataType& upstream ) {
    
    Finalize( upstream.unit(), upstream.sample_rate() );
}