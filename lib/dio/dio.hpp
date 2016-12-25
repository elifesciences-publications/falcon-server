#ifndef DIO_H
#define DIO_H

#include <string>
#include <cstdint>
#include <vector>
#include <stdexcept>


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
    
    std::vector<uint32_t> find_channels( DigitalOutputMode mode = DigitalOutputMode::NONE ) ;
    
    void execute( DigitalDevice & device );
    
protected:
    
    uint32_t nchannels_;
    unsigned int pulse_width_;
    std::vector<DigitalOutputMode> mode_;
};


#endif // DIO_H
