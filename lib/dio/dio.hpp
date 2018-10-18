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

#ifndef DIO_HPP
#define DIO_HPP

#include <string>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <random>
#include <memory>


enum class DigitalOutputMode {NONE, HIGH, LOW, TOGGLE, PULSE};

class DigitalDeviceError : public std::runtime_error {

public:
    DigitalDeviceError( std::string msg ) : runtime_error( msg ) {}
    DigitalDeviceError( std::string const command, std::string error ) :
        std::runtime_error( "Error executing command " + command + " (error = " + error + ")." ) {}
};

class DigitalStateError : public std::runtime_error {

public:
    DigitalStateError( std::string msg ) : runtime_error( msg ) {}
};


class DigitalState {

public:
    DigitalState( uint32_t nchannels = 0 ) : state_(nchannels,false) {}
    
    uint32_t nchannels() const;
    
    typename std::vector<bool>::reference operator[]( uint32_t channel );
    
    bool state( uint32_t channel ) const;
    std::vector<bool>& state();
    std::vector<bool> state( std::vector<uint32_t> channels ) const;
    
    void set_state( uint32_t channel, bool value );
    void set_state( std::vector<uint32_t> channels, bool value );
    void set_state( bool value );
    void set_state( std::vector<bool> values );
    void set_state( std::vector<uint32_t> channels, std::vector<bool> values );
    
    void toggle_state( uint32_t channel );
    void toggle_state( std::vector<uint32_t> channels );

    std::string to_string( std::string high="1", std::string low="0", std::string spacer="") const;

protected:
    std::vector<bool> state_;
};

class DigitalDevice {

public:
    DigitalDevice(std::string type) : type_(type) {}
    
    std::string type() const;
    virtual std::string description() const;
    
    virtual uint32_t nchannels() const = 0;
    
    virtual DigitalState read_state() const = 0;
    virtual void write_state( DigitalState& state ) = 0;

protected:
    std::string type_;;
};

class DigitalOutputProtocol {
    
public:
    DigitalOutputProtocol( uint32_t nchannels, unsigned int pulse_width, DigitalOutputMode default_mode = DigitalOutputMode::NONE );
    
    DigitalOutputMode mode( uint32_t channel ) const { return mode_[channel]; }
    
    void set_mode( uint32_t channel, DigitalOutputMode mode = DigitalOutputMode::NONE);
    void set_mode( std::vector<uint32_t> channels, DigitalOutputMode mode = DigitalOutputMode::NONE);
    
    unsigned int pulse_width() const;
    void set_pulse_width( unsigned int value );
    
    unsigned int requested_delay( uint32_t channel ) const { return delays_[channel]; }
    unsigned int delay() const { return delay_us_; }
    void set_delay( std::vector<unsigned int> delay_values );
    void set_delay( unsigned int delay ) { delay_us_ = delay; }
    
    std::vector<uint32_t> find_channels( DigitalOutputMode mode = DigitalOutputMode::NONE ) ;
    
    uint32_t n_channels() const { return nchannels_;}
    
    void execute( DigitalDevice & device, bool no_delays );
    
protected:
    
    uint32_t nchannels_;
    unsigned int pulse_width_;
    std::vector<DigitalOutputMode> mode_;
    std::vector<unsigned int> delays_;
    unsigned int delay_us_;
    bool fixed_delay_;
    std::default_random_engine generator_;
    std::uniform_int_distribution<unsigned int> distribution_;
};


#endif // DIO_HPP
