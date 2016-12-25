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