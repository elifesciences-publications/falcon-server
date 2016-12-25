#ifndef PORTPOLICY_H
#define PORTPOLICY_H

#include "../ringbuffer.hpp"
#include "utilities/math_numeric.hpp"

typedef uint16_t SlotType;
typedef Range<SlotType> SlotRange;

class PortPolicy {
public:
    PortPolicy( SlotRange slot_number_range = SlotRange(1) ) : 
    slot_number_range_(slot_number_range)
    {}
    
    const SlotRange& slot_number_range() const {return slot_number_range_; }
    SlotType min_slot_number() const { return slot_number_range_.lower(); }
    SlotType max_slot_number() const { return slot_number_range_.upper(); }
    
    bool isdynamic() const { return max_slot_number()>min_slot_number(); }
    
protected:
    SlotRange slot_number_range_;
    
};

class PortInPolicy : public PortPolicy {
public:
    PortInPolicy( SlotRange slot_number_range = SlotRange(1), bool cache = false, int64_t time_out = -1 ) :
    PortPolicy(slot_number_range), cache_enabled_(cache), time_out_(time_out)
    {}
    
    bool cache_enabled() const { return cache_enabled_; }
    int64_t time_out() const { return time_out_; }
    
protected:
    bool cache_enabled_; // input slot only
    int64_t time_out_; // in microseconds, input slot only
};

class PortOutPolicy : public PortPolicy {
public:
    PortOutPolicy( SlotRange slot_number_range = SlotRange(1), int buffer_size = 200, WaitStrategy wait = WaitStrategy::kBlockingStrategy ) :
    PortPolicy(slot_number_range), buffer_size_(buffer_size), wait_strategy_(wait)
    {}
    
    int buffer_size() const { return buffer_size_; }
    WaitStrategy wait_strategy() const { return wait_strategy_; }
    
    void set_buffer_size( int sz ) { buffer_size_ = sz; }

protected:
    int buffer_size_; // output slot only
    WaitStrategy wait_strategy_; // ouput slot only
};

#endif // portpolicy.hpp
