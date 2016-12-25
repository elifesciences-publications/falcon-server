#ifndef ADVANTECHDIO_H
#define ADVANTECHDIO_H

#include "dio.hpp"
#include "advantech/bdaqctrl.h"

#define DEVICE_DESCRIPTION L"USB-4750, BID#0"

class AdvantechDIO : public DigitalDevice {

public:
    AdvantechDIO( int port = -1, uint64_t delay = DEFAULT_DELAY_MICROSEC );
    
    ~AdvantechDIO();
    
    uint32_t nchannels() const;
    
    std::string description() const;
    
    DigitalState read_state() const;
    void write_state( DigitalState& state );

protected:
    void read( std::vector<uint8_t> & values ) const;
    void read( uint32_t port, uint8_t & value ) const;
    
    void write( uint32_t port, uint8_t value ) const;
    void write( std::vector<uint8_t> & values ) const;

protected:
    Automation::BDaq::InstantDoCtrl* device_;
    std::string description_;
    int port_;
    uint64_t delay_;
    uint32_t nports_;
    uint32_t nmaxports_;
    uint32_t nchannels_;
    
    DigitalState state_;
    
public:
    const unsigned int CHANNELS_PER_PORT = 8;
    static const uint64_t DEFAULT_DELAY_MICROSEC = 5;
};

std::string advantech_error_to_string( Automation::BDaq::ErrorCode_DIO error_code );

#endif // ADVANTECHDIO_H

