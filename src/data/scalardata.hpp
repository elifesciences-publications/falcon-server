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

#ifndef SCALARDATA_H
#define SCALARDATA_H

#include "idata.hpp"
#include "utilities/string.hpp"

#define DEFAULT_SCALAR_VALUE 0 // can be used in a template

template <class TYPE>
class ScalarData : public IData {
    
public:
    ScalarData( TYPE data = DEFAULT_SCALAR_VALUE ) : data_(data) {}
    
    virtual void ClearData() override {}
    
    TYPE const & data() const { return data_; }
    
    void set_data( const TYPE& data ) { data_ = data; }
    
    void set_data( const ScalarData<TYPE> &source ) {data_ = source.data();}
    
    friend bool operator == (ScalarData<TYPE>  &a, ScalarData<TYPE>  &b) {return a.data == b.data;}
   
    friend bool operator != (ScalarData<TYPE>  &a, ScalarData<TYPE>  &b) {return a.data != b.data;}
    
    virtual void SerializeBinary( std::ostream& stream,
    Serialization::Format format = Serialization::Format::FULL ) const override {

        IData::SerializeBinary( stream, format );
        if (format==Serialization::Format::FULL || format==Serialization::Format::COMPACT) {
            stream.write( reinterpret_cast<const char*>( &data_) , sizeof(TYPE) );
        }
    }
    
    virtual void SerializeYAML( YAML::Node & node,
    Serialization::Format format = Serialization::Format::FULL ) const  override {
        
        IData::SerializeYAML( node, format );
        if (format==Serialization::Format::FULL || format==Serialization::Format::COMPACT) {
            node["scalar_data"] = data_;
        }
    }
    
    virtual void YAMLDescription( YAML::Node & node,
        Serialization::Format format = Serialization::Format::FULL ) const override {
        
        IData::YAMLDescription( node, format );
        if (format==Serialization::Format::FULL || format==Serialization::Format::COMPACT) {
            node.push_back( "scalar_data " +  get_type_string<TYPE>() + " (1)" );
        }
    }
    
protected:
    TYPE data_;
};

template <class TYPE>
class ScalarDataType : public AnyDataType {

ASSOCIATED_DATACLASS(ScalarData<TYPE>)
    
public:
    ScalarDataType( TYPE default_data = DEFAULT_SCALAR_VALUE ) : AnyDataType(true), default_data_( default_data ) { }
    
    TYPE default_data() const {return default_data_;}
    
    void InitializeData( ScalarData<TYPE>& item ) const { item.set_data(default_data_); }
    
    virtual std::string name() const { return "scalar"; }
    
protected:
    TYPE default_data_;

};


#endif // scalardata.hpp
