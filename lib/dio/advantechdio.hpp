// ---------------------------------------------------------------------
// This file is part of falcon-server.
// 
// Copyright (C) 2015, 2016, 2017 Neuro-Electronics Research Flanders
// 
// Falcon-server is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Falcon-server is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with falcon-server. If not, see <http://www.gnu.org/licenses/>.
// ---------------------------------------------------------------------

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

