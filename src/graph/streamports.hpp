#ifndef STREAMPORTS_H
#define STREAMPORTS_H

#include "istreamports.hpp"
#include "connections.hpp"
#include <set>

struct RingBufferStatus {
    uint64_t read;
    uint64_t backlog;
    bool alive;
};

//forward declarations
template <typename DATATYPE>
class SlotIn;

template <typename DATATYPE>
class PortOut;

template <typename DATATYPE>
class PortIn;

int IdentifyNextSlot( int slot_request, int connected_slot_number, bool allow_multi_connect, const PortPolicy& policy );

template <typename DATATYPE>
class SlotOut : public ISlotOut {

friend class PortOut<DATATYPE>;

public:
    SlotOut( DATATYPE datatype ) : streaminfo_(datatype), ringbuffer_serial_number_(0) {}
    
    // public interface
    typename DATATYPE::DATACLASS* ClaimData( bool clear );
    std::vector<typename DATATYPE::DATACLASS*> ClaimDataN( uint64_t n, bool clear );
    void PublishData();
    
    DATATYPE& datatype() { return streaminfo_.datatype(); }
    virtual StreamInfo<DATATYPE>& streaminfo() { return streaminfo_; }
    
    uint64_t nitems_produced() const;
    
protected:
	// called by SlotIn<DATATYPE>
    virtual typename DATATYPE::DATACLASS* DataAt( int64_t sequence ) const { return ringbuffer_->Get( sequence ); }
    
    void CreateRingBuffer(int buffer_size, WaitStrategy wait_strategy);
    void Unlock();
    
    RingBatch* next_batch( uint64_t n = 1 );
	
	virtual void PrepareProcessing() {
        
        ringbuffer_serial_number_ = 0;
        
        if (!connected()) {return;}
        
        ringbuffer_->ForcePublish( -1L );
        ringbuffer_->Claim( -1L );
    }
    
public:
    StreamInfo<DATATYPE> streaminfo_; // owned by SlotOut, once finalized, the streaminfo (and datatype) are fixed for the life time of the slot(?)
    std::unique_ptr< DataFactory<DATATYPE> > datafactory_ = nullptr;
    std::unique_ptr< RingBuffer<typename DATATYPE::DATACLASS> > ringbuffer_ = nullptr;

protected:
    uint64_t ringbuffer_serial_number_;
};

template <typename DATATYPE>
class PortOut : public IPortOut {
public:
    PortOut( std::string name, DATATYPE datatype, PortOutPolicy policy ) : IPortOut(name, policy), datatype_(datatype) {
        NewSlot( policy.min_slot_number() );
    }
    
    virtual SlotType number_of_slots() const override { return slots_.size(); }
    
    virtual const AnyDataType& datatype() const override { return datatype_; }
    
    StreamInfo<DATATYPE>& streaminfo( std::size_t index ) { return slots_[index]->streaminfo(); }
    
    virtual SlotOut<DATATYPE>* slot( std::size_t index ) { return slots_[index].get(); }
    
    SlotOut<DATATYPE>* dataslot( std::size_t index ) { return slots_[index].get(); }
    
    
protected:
    //called by StreamOutConnector
    virtual void Connect (int slot, StreamInConnector* downstream ) override;
    virtual int ReserveSlot( int slot ) override;
    
    //called by IPortOut
    virtual void CreateRingBuffers() override;
    virtual void UnlockSlots() override;
    
    //called by PortOut<DATATYPE>::Connect
    virtual void NewSlot(int n=1);
	
	virtual void PrepareProcessing() override {
        for (auto& it : slots_) {
            it->PrepareProcessing();
        }
    }

private:
    DATATYPE datatype_;
    std::vector<std::unique_ptr<SlotOut<DATATYPE>>> slots_;
};

template <typename DATATYPE>
class SlotIn : public ISlotIn {

friend class PortIn<DATATYPE>;

public:
    SlotIn( DATATYPE datatype, int64_t time_out = -1, bool cache = false ) : ISlotIn(time_out,cache), datatype_(datatype) {}
	
	// methods called by processor implementation
    const typename DATATYPE::DATACLASS* GetDataPrototype() const;
    bool RetrieveData( typename DATATYPE::DATACLASS* & data );
    bool RetrieveDataN( uint64_t n, std::vector<typename DATATYPE::DATACLASS*> & data );
    bool RetrieveDataAll( std::vector<typename DATATYPE::DATACLASS*> & data );
    
    StreamInfo<DATATYPE>& streaminfo() {
        if (!connected()) {
            throw std::runtime_error( "Input slot is not connected" );
        }
        
        return (StreamInfo<DATATYPE>&) upstream_connector_->streaminfo();
    }
    
    bool status_alive() const { return status_.alive; }
    uint64_t status_read() const { return status_.read; }
    uint64_t status_backlog() const { return status_.backlog; }
    
protected:
    void Unlock();
    void check_high_water_level();
    
    RingBufferStatus status_;
    
    const double HIGH_WATER_LEVEL = 0.85;
    unsigned int n_messages_;
    const unsigned int MAX_N_MESSAGES = 20;
    
    DATATYPE datatype_;
    
public:
    typename DATATYPE::DATACLASS* cache_;
};

template <typename DATATYPE>
class PortIn : public IPortIn {
public:
    PortIn( std::string name, DATATYPE datatype, PortInPolicy policy ) : IPortIn(name, policy), datatype_(datatype) {
        NewSlot( policy.min_slot_number() );
    }
    
    virtual SlotType number_of_slots() const override { return slots_.size(); }
    
    virtual SlotIn<DATATYPE>* slot( std::size_t index ) { return slots_[index].get(); }
    SlotIn<DATATYPE>* dataslot( std::size_t index ) { return slots_[index].get(); }
    
    virtual const AnyDataType& datatype() const override { return datatype_; }
    
    StreamInfo<DATATYPE>& streaminfo( std::size_t index ) { return slots_[index]->streaminfo(); }
    
    virtual void PrepareProcessing() override {
        for (auto& it : slots_) {
            it->PrepareProcessing();
        }
    }
    
protected:
    //called by StreamInConnector
    virtual void Connect( int slot, StreamOutConnector* upstream );
    virtual int ReserveSlot( int slot );
    virtual bool CheckCompatibility( IPortOut* upstream );
    
    virtual void UnlockSlots() override;
    
    void NewSlot( int n=1 );
    
private:
    DATATYPE datatype_;
    std::vector<std::unique_ptr<SlotIn<DATATYPE>>> slots_;
};


// TODO: include implementation here!!
#include "streamports.ipp"

#endif
