#ifndef ISTREAMPORTS_H
#define ISTREAMPORTS_H

#include "portpolicy.hpp"
#include "../data/idata.hpp"
#include "connections.hpp"

#include "yaml-cpp/yaml.h"

//forward declarations
//class StreamInConnector;
//class StreamOutConnector;

class ISlotOut {

friend class ISlotIn;

template <typename DATATYPE>
friend class SlotIn;

friend class IPortOut;

public:
	ISlotOut() : ring_batch_(1), buffer_size_(-1) {}
	
    bool connected() const { return downstream_slots_.size()>0; }
    int nconnected() const { return downstream_slots_.size(); }
    
    virtual IStreamInfo& streaminfo() = 0;
    
    int buffer_size() const { return buffer_size_; }
    
protected:
	// called by IPortOut
	void Connect( StreamInConnector* downstream );

	// called by SlotIn
    int64_t WaitFor( int64_t sequence ) const { return barrier_->WaitFor( sequence ); }
    int64_t WaitFor( int64_t sequence, int64_t time_out ) const { return barrier_->WaitFor( sequence, time_out ); }
	
    virtual IData* DataAt( int64_t sequence ) const = 0;
    
    
	std::vector<RingSequence*> gating_sequences();

    
protected:
	RingBatch ring_batch_;
	bool has_publishable_data_ = false;
	
	std::set<ISlotIn*> downstream_slots_; // need to go through base class, since we don't know the exact datatype of downstream slots
	
	std::unique_ptr< RingBarrier > barrier_ = nullptr;
	
    int buffer_size_;
};

class IPortOut {

friend class StreamOutConnector;
friend class IProcessor;

public:
    IPortOut( std::string name, PortOutPolicy policy ) : name_(name), policy_(policy) {}
    
    const PortOutPolicy& policy() const { return policy_; }
    
    virtual const AnyDataType& datatype() const = 0;
    
    virtual ISlotOut* slot( std::size_t index ) = 0;
    virtual SlotType number_of_slots() const = 0;
	
    YAML::Node ExportYAML() const;
    
    std::string name() const { return name_; }
    
protected:
    //called by StreamOutConnector
    virtual void Connect( int slot, StreamInConnector* downstream ) = 0;
    virtual int ReserveSlot( int slot ) = 0;
    
    virtual void CreateRingBuffers() = 0;
    virtual void UnlockSlots() = 0;
	
	virtual void PrepareProcessing() = 0;
    
    virtual void NewSlot( int n=1 ) = 0;
    
    void set_buffer_size( int sz ) {
        policy_.set_buffer_size( sz );
    }
    
private:
    std::string name_;
    PortOutPolicy policy_;
};

class ISlotIn {

friend class IPortIn;

template <typename DATATYPE>
friend class PortIn;

friend class ISlotOut;

public:
    ISlotIn( int64_t time_out = -1, bool cache = false) : time_out_(time_out), cache_enabled_(cache) {}
    
    bool connected() const { return upstream_!=nullptr; };
    
	void ReleaseData();
    
    const SlotAddress& upstream_address() { 
        
        if (upstream_connector_ == nullptr) {
            throw std::runtime_error( "Cannot get upstream address: slot is not connected." );
        }
        return upstream_connector_->address();
    }
	
    const PortOutPolicy& upstream_policy() const { 
        
        if (upstream_connector_ == nullptr) {
            throw std::runtime_error( "Cannot get upstream policy: slot is not connected." );
        }
        return upstream_connector_->port()->policy();
    }
    
protected:
	// called by upstream ISlotOut
	RingSequence* sequence() { return &sequence_; }
	
	//called by IPortIn
    void Connect( StreamOutConnector* upstream );
	void PrepareProcessing();
	
protected:
    int64_t time_out_;
    bool cache_enabled_;
	
	int64_t ncached_=0;
    int64_t nretrieved_=0;
	
	IData* cache_ = nullptr;
	
	ISlotOut* upstream_ = nullptr; // access to upstream slot needs to go through base pointer (since we don't know the exact datatype)
	StreamOutConnector* upstream_connector_ = nullptr;
    
	RingSequence sequence_; // the input slot's read cursor into the buffer
	
	
};

class IPortIn {

friend class StreamInConnector;
friend class IProcessor;
    
public:
    IPortIn( std::string name, PortInPolicy policy ) : name_(name), policy_(policy) {}
    
    const PortInPolicy& policy() const { return policy_; }
    
    virtual const AnyDataType& datatype() const = 0;
    
	virtual SlotType number_of_slots() const = 0;
	
    virtual ISlotIn* slot( std::size_t index ) = 0;
    
    YAML::Node ExportYAML() const;
    
    std::string name() const { return name_; }
    
protected:
    // called by StreamInConnector
    virtual void Connect( int slot, StreamOutConnector* upstream ) = 0;
    virtual int ReserveSlot( int slot ) = 0;
    virtual bool CheckCompatibility( IPortOut* upstream ) = 0;
    // called by ...
	virtual void PrepareProcessing() = 0;
    
    virtual void UnlockSlots() = 0;
	
private:
    std::string name_;
    PortInPolicy policy_;
};

#endif
