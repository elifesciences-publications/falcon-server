#include "dummydio.hpp"

uint32_t DummyDIO::nchannels() const {
    
    return state_.nchannels();
}
    
DigitalState DummyDIO::read_state() const {
    
    return state_;
}

void DummyDIO::write_state( DigitalState& state ) {
    
    state_ = state;
}
