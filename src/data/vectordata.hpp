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

#ifndef VECTORDATA_HPP
#define VECTORDATA_HPP

#include "idata.hpp"
#include <vector>

template <class T>
class VectorData : public IData {

public:    
    
    void Initialize() {};
    
    virtual void ClearData() override {
        
        data_.assign( data_.size(), 0 );
    }
    
    void set_data( const std::vector<T>& data ) {
        
        data_ = data;
    }
    
    void set_data( const T* data, int len ) {
        
        std::copy( data, data + len, data_.begin() );
    }
    
    void set_sample( int index, const T& data ) {
        
        data_[index] = data;
    }
    
    const std::vector<T> data() {
        
        return data_;
    }
    
    T* data_array() {
        
        return data_.data();
    }
    
    std::size_t n_elem() const {
        
        return data_.size();
    }
    
    void reserve( std::size_t n ) {
        
        data_.reserve(n);
    }
    
    virtual void YAMLDescription( YAML::Node & node,
        Serialization::Format format = Serialization::Format::FULL ) const override {
    
        node.push_back( "vector_data " + get_type_string<T>() +
            " (" + std::to_string(data_.size()) + ")" );
    }
    
    virtual void SerializeBinary( std::ostream& stream,
        Serialization::Format format = Serialization::Format::FULL ) const override {
        
        IData::SerializeBinary( stream, format );
        if (format==Serialization::Format::FULL || format==Serialization::Format::COMPACT) {
            stream.write( reinterpret_cast<const char*>( &data_), sizeof(T)*data_.size() );
        }
    }
    
    virtual void SerializeYAML( YAML::Node & node,
        Serialization::Format format = Serialization::Format::FULL ) const override {
        
        IData::SerializeYAML( node, format );
        if (format==Serialization::Format::FULL || format==Serialization::Format::COMPACT) {
            node["vector_data"] = data_;
        }
    }
    
protected:
    std::vector<T> data_;
};

template <class T>
class VectorDataType : public AnyDataType {

ASSOCIATED_DATACLASS(VectorData<T>)
    
public:
    VectorDataType( std::size_t n = 1 ) : size_(n) { }
        
    void InitializeData( VectorData<T>& item ) const {
        
        item.reserve( size_ );
    }
    
    std::size_t size() const {
        
        return size_;
    }

    bool CheckCompatibility( const VectorDataType<T>& upstream ) const {      
    
        return (size_ == upstream.size());
    }
    
protected:
    std::size_t size_;
};


#endif // vectordata.hpp
