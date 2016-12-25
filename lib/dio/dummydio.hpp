#ifndef DUMMYDIO_H
#define DUMMYDIO_H

#include "dio.hpp"

class DummyDIO : public DigitalDevice {

public:
    DummyDIO( uint32_t nchannels ) : DigitalDevice("dummy"), state_(nchannels) {}
    
    uint32_t nchannels() const;
    
    DigitalState read_state() const;
    void write_state( DigitalState& state );
    
protected:
    DigitalState state_;
};


#endif // DUMMYDIO_H
