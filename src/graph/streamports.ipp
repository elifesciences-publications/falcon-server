//#include "streamports.hpp"
//#include "g3log/src/g2log.hpp"


template<typename DATATYPE>
inline uint64_t SlotOut<DATATYPE>::nitems_produced() const {
    
    return ringbuffer_serial_number_;
}

template <typename DATATYPE>
inline typename DATATYPE::DATACLASS* SlotOut<DATATYPE>::ClaimData( bool clear ) {
    
    next_batch(1);
    has_publishable_data_ = true;
    typename DATATYPE::DATACLASS* data = ringbuffer_->Get( ring_batch_.Start() );
    if (clear) { data->ClearData(); }
    data->set_serial_number( ringbuffer_serial_number_++ );
    return data;
}

template <typename DATATYPE>
inline std::vector<typename DATATYPE::DATACLASS*> SlotOut<DATATYPE>::ClaimDataN( uint64_t n , bool clear) {
    
    std::vector<typename DATATYPE::DATACLASS*> data;
    
    next_batch( n );
    int64_t start = ring_batch_.Start();
    
    for (int64_t k=start; k<=ring_batch_.end(); k++) {
        data.push_back( (typename DATATYPE::DATACLASS*) ringbuffer_->Get( k ) );
    }
    
    if (clear) {
        for (auto& it : data) {
            it->ClearData();
        }
    }
    
    for (auto& it : data) {
        it->set_serial_number( ringbuffer_serial_number_++ );
    }
    
    has_publishable_data_ = true;
    
    return data;
}

template <typename DATATYPE>
inline void SlotOut<DATATYPE>::PublishData() { 
    
    if (has_publishable_data_ && ringbuffer_->GetCursor()!=INT64_MAX) {
        ringbuffer_->Publish( ring_batch_ );
        has_publishable_data_ = false;
    }
}

template <typename DATATYPE>
void SlotOut<DATATYPE>::CreateRingBuffer( int buffer_size, WaitStrategy wait_strategy ) {
    
    // make sure buffer size is power of 2 and at least 2
    buffer_size_ = buffer_size<2 ? 2 : next_pow2( buffer_size );
    datafactory_.reset( new DataFactory<DATATYPE>( streaminfo_.datatype() ) );
    try {
        ringbuffer_.reset( new RingBuffer<typename DATATYPE::DATACLASS>( datafactory_.get() , buffer_size_, ClaimStrategy::kSingleThreadedStrategy, wait_strategy ) );
    } catch (std::runtime_error & e) {
        throw;
    }
    barrier_.reset( ringbuffer_->NewBarrier( std::vector<RingSequence*>(0) ) );
    ringbuffer_->set_gating_sequences( gating_sequences() );
}

template <typename DATATYPE>
void SlotOut<DATATYPE>::Unlock() {
    
    if (connected()) {
        ringbuffer_->ForcePublish( INT64_MAX );    
    }
}

template <typename DATATYPE>
inline RingBatch* SlotOut<DATATYPE>::next_batch( uint64_t n ) { 
    
    ring_batch_.set_size( (int) n );
    ring_batch_.set_end( -1 );
    return ringbuffer_->Next(&ring_batch_);
}

template <typename DATATYPE>
void PortOut<DATATYPE>::Connect (int slot, StreamInConnector* downstream ) {
    
    //if ( slot>policy().min_slot_number() && slot == number_of_slots()+1 && slot <= policy().max_slot_number()) {
    //    // create new slot
    //    this->NewSlot();
    //}
    
    if (slot<0 || slot>=number_of_slots()) {
        throw std::out_of_range( "Error connecting to slot " + std::to_string(slot) + " (invalid slot number)" );
    }
    
    this->slots_.at(slot)->Connect( downstream );
}

template <typename DATATYPE>
int PortOut<DATATYPE>::ReserveSlot( int slot ) {
    
    int open_slot = -1;
    int nconnections = 0;
    
    for (int n=0; n<(int)slots_.size(); n++) {
        nconnections += slots_[n]->nconnected();
        if ( open_slot==-1 && !slots_[n]->connected() ) { open_slot=n; }
    }
    
    if (open_slot==-1) {
        open_slot = nconnections;
    }
    
    int reserved_slot = IdentifyNextSlot( slot, open_slot, true, policy());
    
    if (reserved_slot<0) { throw std::runtime_error("Cannot reserve slot."); }
    
    if (reserved_slot==number_of_slots()) { this->NewSlot(); };
    
    return reserved_slot;
}

template <typename DATATYPE>
void PortOut<DATATYPE>::CreateRingBuffers() {
    
    for (auto& slot_it : slots_) {
        slot_it->CreateRingBuffer(policy().buffer_size(), policy().wait_strategy());
    }
}

template <typename DATATYPE>
void PortOut<DATATYPE>::UnlockSlots() {
    
    for (auto& slot_it : slots_) {
        slot_it->Unlock();
    }
}

template <typename DATATYPE>
void PortOut<DATATYPE>::NewSlot(int n) {
    
    for( int k=0; k<n; k++ ) {
        this->slots_.push_back( std::unique_ptr<SlotOut<DATATYPE>>( new SlotOut<DATATYPE>(this->datatype_) ) );
    }
    //LOG(DEBUG) << "Creating new slot. dt=" << typeid(DATATYPE).name();
}

template <typename DATATYPE>
const typename DATATYPE::DATACLASS* SlotIn<DATATYPE>::GetDataPrototype() const {
    
    const typename DATATYPE::DATACLASS* data = nullptr;
    data = (const typename DATATYPE::DATACLASS*) upstream_->DataAt( 0 );
    return data;
}

template <typename DATATYPE>
void SlotIn<DATATYPE>::check_high_water_level() {
    if ( status_.backlog > HIGH_WATER_LEVEL*upstream_->buffer_size() and n_messages_ == 0) {
        LOG(WARNING) << "high-water level reached for " << upstream_address().string();
        ++ n_messages_;
        if ( n_messages_ == MAX_N_MESSAGES ) {
            n_messages_ = 0;
        }
    }
}

template <typename DATATYPE>
bool SlotIn<DATATYPE>::RetrieveData( typename DATATYPE::DATACLASS* & data ) {
    
    data = nullptr;
    status_.read = status_.backlog = 0;
    status_.alive = true;
    
    if (!connected()) { return status_.alive; }
    
    int64_t requested_sequence = sequence_.sequence();
    if (requested_sequence==INT64_MAX) { status_.alive = false; return status_.alive; }
    requested_sequence += ncached_ + 1L;
    
    try {
        if (time_out_ < 0) {
            int64_t available_sequence = upstream_->WaitFor( requested_sequence );
            if (available_sequence==INT64_MAX) {
                status_.alive = false;
            } else {
                data = (typename DATATYPE::DATACLASS*) upstream_->DataAt( requested_sequence );
                ++nretrieved_;
                status_.read = 1;
                status_.backlog = available_sequence - requested_sequence;
            }
        } else {
            int64_t available_sequence = upstream_->WaitFor( requested_sequence, time_out_ );
            
            if (available_sequence < requested_sequence) {
                // timed out
                if (cache_enabled_) { data = cache_; status_.read = 1; }
            } else if (available_sequence==INT64_MAX) {
                status_.alive = false;
            } else {
                
                data = (typename DATATYPE::DATACLASS*) upstream_->DataAt( requested_sequence );
                ++nretrieved_;
                status_.read = 1;
                status_.backlog = available_sequence - requested_sequence;
                
                if (cache_enabled_) {
                    if (ncached_==0) {--nretrieved_;}
                    cache_ = data;
                    ncached_ = 1;
                }
            }
        }
    } catch ( const RingAlertException& e) {
        // terminate processing
        status_.alive = false;
    }
    
    
    check_high_water_level();
    
    return status_.alive;
    
}

template <typename DATATYPE>
bool SlotIn<DATATYPE>::RetrieveDataN( uint64_t n, std::vector<typename DATATYPE::DATACLASS*> & data ) {
    
    // will only cache last value, but does not return cached values when timed out if n>1
    
    data.clear();
    status_.read = status_.backlog = 0;
    status_.alive = true;
    
    if (!connected() || n==0) { return status_.alive; }
      
    int64_t current_sequence = sequence_.sequence();
    if (current_sequence==INT64_MAX) { status_.alive = false; return status_.alive; }
    current_sequence += ncached_;
    int64_t requested_sequence = current_sequence + n;
    
    try {
        if (time_out_ < 0) {
            int64_t available_sequence = upstream_->WaitFor( requested_sequence );
            if (available_sequence==INT64_MAX) {
                status_.alive = false;
            } else {
                for (int64_t k=current_sequence+1; k<=requested_sequence; k++) {
                    data.push_back( (typename DATATYPE::DATACLASS*) upstream_->DataAt( k ) );
                    ++nretrieved_;
                    ++status_.read;
                }
                status_.backlog = available_sequence - requested_sequence;
            }
        } else {
            int64_t available_sequence = upstream_->WaitFor( requested_sequence, time_out_ );
            
            if (available_sequence < requested_sequence) {
                // timed out
                if (n==1 && cache_enabled_) { data.push_back( cache_ ); status_.read=1; }
            } else if (available_sequence==INT64_MAX) {
                status_.alive = false;
            } else {
                
                for (int64_t k=current_sequence+1; k<=requested_sequence; k++) {
                    data.push_back( (typename DATATYPE::DATACLASS*) upstream_->DataAt( k ) );
                    ++nretrieved_;
                    ++status_.read;
                }
                
                status_.backlog = available_sequence - requested_sequence;
                
                if (cache_enabled_) {
                    if (ncached_==0) {--nretrieved_;}
                    cache_ = data.back();
                    ncached_ = 1;
                }
            }
        }
    } catch (const RingAlertException& e) {
        status_.alive = false;
    }
    
    check_high_water_level();
    
    return status_.alive;
}

template <typename DATATYPE>
bool SlotIn<DATATYPE>::RetrieveDataAll( std::vector<typename DATATYPE::DATACLASS*> & data ) {
    
    // supports single item caching
      
    data.clear();
    status_.read = status_.backlog = 0;
    status_.alive = true;
    
    if (!connected()) { return status_.alive; }
    
    int64_t current_sequence = sequence_.sequence();
    if (current_sequence==INT64_MAX) { status_.alive = false; return status_.alive; }
    current_sequence += ncached_;
    int64_t requested_sequence = current_sequence + 1L;
   
    try {
        if (time_out_ < 0) {
            int64_t available_sequence = upstream_->WaitFor( requested_sequence );
            if (available_sequence==INT64_MAX) {
                status_.alive = false;
            } else {
                for (int64_t k=current_sequence+1; k<=available_sequence; k++) {    
                    data.push_back( (typename DATATYPE::DATACLASS*) upstream_->DataAt( k ) );
                    ++nretrieved_;
                    ++status_.read;
                }
            }
        } else {
            int64_t available_sequence = upstream_->WaitFor( requested_sequence, time_out_ );
            if (available_sequence < requested_sequence) {
                // timed out
                if (cache_enabled_) { data.push_back( cache_ ); status_.read=1; }
            } else if (available_sequence==INT64_MAX) {
                status_.alive = false;
            } else {
                
                for (int64_t k=current_sequence+1; k<=available_sequence; k++) {
                    data.push_back( (typename DATATYPE::DATACLASS*) upstream_->DataAt( k ) );
                    ++nretrieved_;
                    ++status_.read;
                }
            
                if (cache_enabled_) {
                    if (ncached_==0) {--nretrieved_;}
                    cache_ = data.back();
                    ncached_ = 1;
                }
            }
            
        }
    } catch (const RingAlertException& e) {
        status_.alive = false;
    }
    
    check_high_water_level();
    
    return status_.alive;
    
}

template <typename DATATYPE>
void SlotIn<DATATYPE>::Unlock() {
    
    sequence_.set_sequence( INT64_MAX );
}

template <typename DATATYPE>
void PortIn<DATATYPE>::Connect( int slot, StreamOutConnector* upstream ) {
    
    if (slot>=policy().min_slot_number() && slot == number_of_slots() && slot < policy().max_slot_number()) {
        // create new slot
        NewSlot();
    }
    
    if (slot<0 || slot>=number_of_slots()) {
       throw std::out_of_range( "Error connecting to slot " + std::to_string(slot) + " (number of available slots = " + std::to_string(number_of_slots()) + ")" );
    }
    
    slots_.at(slot)->Connect( upstream );
}

template <typename DATATYPE>
int PortIn<DATATYPE>::ReserveSlot( int slot ) {
    
    int nconnected;
    
    for (nconnected=0; nconnected<(int)slots_.size(); nconnected++) {
        if ( !slots_[nconnected]->connected() ) { break; }
    }
    
    int reserved_slot = IdentifyNextSlot( slot, nconnected, false, policy());
    
    if (reserved_slot<0) { throw std::runtime_error("Cannot reserve slot."); }
    
    if (reserved_slot==number_of_slots()) { this->NewSlot(); };
    
    return reserved_slot;
    
}

template <typename DATATYPE>
bool PortIn<DATATYPE>::CheckCompatibility( IPortOut* upstream ) {
    
    return CheckDataType( datatype_, upstream->datatype() );
}

template <typename DATATYPE>
void PortIn<DATATYPE>::NewSlot( int n ) {
    
    for (int k=0; k<n; k++) {
        slots_.push_back( std::move( std::unique_ptr<SlotIn<DATATYPE>>( new SlotIn<DATATYPE>(datatype_, policy().time_out(), policy().cache_enabled() ) ) ) );
    }
}

template <typename DATATYPE>
void PortIn<DATATYPE>::UnlockSlots() {
    
    for (auto& slot_it : slots_) {
        slot_it->Unlock();
    }
}
