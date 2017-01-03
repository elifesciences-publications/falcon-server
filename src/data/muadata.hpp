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

#ifndef MUADATA_HPP
#define	MUADATA_HPP

#include "idata.hpp"


class MUAData : public IData {
public:
    void Initialize( double bin_size );
    
    virtual void ClearData() override;
    
    void set_n_spikes( unsigned int n_spikes );
    
    double mua() const;
    
    void set_bin_size( double bin_size );
    
    double bin_size();
    
    unsigned int n_spikes();
    
    virtual void SerializeBinary( std::ostream& stream,
        Serialization::Format format = Serialization::Format::FULL ) const override final;
    
    virtual void SerializeYAML( YAML::Node & node,
        Serialization::Format format = Serialization::Format::FULL ) const override final;
    
    virtual void YAMLDescription( YAML::Node & node,
        Serialization::Format format = Serialization::Format::FULL ) const override final;
    
protected:
    double bin_size_; // in ms
    unsigned int n_spikes_;
};


class MUADataType : public AnyDataType {

ASSOCIATED_DATACLASS(MUAData);

public:
	
    MUADataType() {}

    double bin_size() const;
	
    virtual void InitializeData( MUAData& item ) const;
    
    virtual void Finalize( double bin_size );

protected:
    double bin_size_;

};

#endif	// muadata.hpp
