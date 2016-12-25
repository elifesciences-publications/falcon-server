#ifndef EVENTDATA_HPP
#define EVENTDATA_HPP

#include "idata.hpp"

typedef unsigned int EventIDType;

const std::string DEFAULT_EVENT = "none";

class EventData : public IData {

public:
    
    EventData( std::string event = DEFAULT_EVENT );
    
    void Initialize( std::string event = DEFAULT_EVENT );
    
    virtual void ClearData() override;
    
    std::string event() const;
    size_t hash() const;
    
    void set_event( std::string event );
    void set_event( const EventData &source );
    
    friend bool operator==(EventData &e1, EventData &e2);
    friend bool operator!=(EventData &e1, EventData &e2);
    
    virtual void SerializeBinary( std::ostream& stream,
        Serialization::Format format = Serialization::Format::FULL ) const override;
    virtual void SerializeYAML( YAML::Node & node,
        Serialization::Format format = Serialization::Format::FULL ) const  override;
    virtual void YAMLDescription( YAML::Node & node,
        Serialization::Format format = Serialization::Format::FULL ) const override;
    
protected:
    std::string event_;
    size_t hash_;
    
    static const unsigned int EVENT_STRING_LENGTH = 128;
};


class EventDataType : public AnyDataType {

ASSOCIATED_DATACLASS(EventData)

public:
    EventDataType( std::string default_event = DEFAULT_EVENT ) :
        AnyDataType(true), default_event_( default_event ) { }
    
    std::string default_event() const;
    
    virtual void InitializeData( EventData& item ) const;
    
    virtual std::string name() const { return DEFAULT_EVENT; }
        
protected:
    std::string default_event_;
};

#endif // eventdata.hpp
