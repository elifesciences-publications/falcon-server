#include "istreamports.hpp"

void ISlotOut::Connect( StreamInConnector* downstream ) {
    
    ISlotIn* p = (ISlotIn*) downstream->slot();
    if ( downstream_slots_.count( p )==0 ) {
        downstream_slots_.insert( p );
    } else {
        throw std::runtime_error("Attempting to connect input slot twice.");
    }
}

std::vector<RingSequence*> ISlotOut::gating_sequences() {
    
    std::vector<RingSequence*> v;
    for (auto & it : downstream_slots_ ) {
        v.push_back( it->sequence() );
    }
    return v;
}

void ISlotIn::ReleaseData() {
    
    if (nretrieved_>0) {
        
        int64_t value = sequence_.IncrementAndGet( nretrieved_ );
        nretrieved_ = 0;
        
        if (value+1<0) {sequence_.set_sequence( INT64_MAX );}
        
    }
}

void ISlotIn::Connect( StreamOutConnector* upstream ) {
    
    if (connected()) {
        throw std::runtime_error( "Error connecting to slot (already connected)" );
    }

    upstream_connector_ = upstream;
    upstream_ = (ISlotOut*) upstream->slot();
}

void ISlotIn::PrepareProcessing() {
    
    sequence_.set_sequence(-1L);
    ncached_ = 0;
    cache_ = nullptr;
    nretrieved_ = 0;
}

YAML::Node IPortOut::ExportYAML() const {
    YAML::Node node;
    node["datatype"] = datatype().name();
    node["nslots_min"] = policy().min_slot_number();
    node["nslots_max"] = policy().max_slot_number();
    node["buffer_size"] = policy().buffer_size();
    if (policy().wait_strategy()==WaitStrategy::kBlockingStrategy) {
        node["wait_strategy"] = "blocking";
    } else if (policy().wait_strategy()==WaitStrategy::kSleepingStrategy) {
        node["wait_strategy"] = "sleeping";
    } else if (policy().wait_strategy()==WaitStrategy::kYieldingStrategy) {
        node["wait_strategy"] = "yielding";
    } else {
        node["wait_strategy"] = "busy spin";
    }
    return node;
    
}

YAML::Node IPortIn::ExportYAML() const {
    YAML::Node node;
    node["datatype"] = datatype().name();
    node["nslots_min"] = policy().min_slot_number();
    node["nslots_max"] = policy().max_slot_number();
    node["cache"] = policy().cache_enabled();
    node["time_out"] = policy().time_out();
    return node;
}
