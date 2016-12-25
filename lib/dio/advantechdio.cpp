#include "advantechdio.hpp"

#include <locale>
#include <codecvt>
#include <iostream>
#include "utilities/time.hpp"

AdvantechDIO::AdvantechDIO( int port, uint64_t delay ) 
      : DigitalDevice("advantech"), port_(port), delay_(delay) {
    
    device_ = Automation::BDaq::AdxInstantDoCtrlCreate();
    Automation::BDaq::DeviceInformation devInfo( DEVICE_DESCRIPTION );

    Automation::BDaq::ErrorCode_DIO ret;
    ret = device_->setSelectedDevice(devInfo);
    
    if (ret != 0) {
        throw std::runtime_error(
            "Unable to initialize Advantech DIO-Card with error: "
            + advantech_error_to_string(ret) );
    }
    
    nmaxports_ = device_->getPortCount();
    
    if (port_>=0) {
        if (static_cast<uint32_t>(port_)>=nmaxports_) {
            throw std::runtime_error( "Request for non-existing port on Advantech DIO-Card.");
        }
        nports_ = 1;
        nchannels_ = CHANNELS_PER_PORT;
    } else {
        nports_ = nmaxports_;
        nchannels_ = device_->getFeatures()->getChannelCountMax();
    }
    
    state_ = DigitalState( nchannels_ );
}
    
AdvantechDIO::~AdvantechDIO() {
    device_->Cleanup();
    device_->Dispose();
}

uint32_t AdvantechDIO::nchannels() const {
    
    return nchannels_;
}

std::string AdvantechDIO::description() const {
    
    if (port_<0) {
        return type() + " " + description_ + " with imposed communication delay of " + std::to_string(delay_) + " microseconds (" + std::to_string(nchannels()) + " channels on all " + std::to_string(nports_) + " ports)";
    } else {
        return type() + " " + description_ + " with imposed communication delay of " + std::to_string(delay_) + " microseconds (" + std::to_string(nchannels()) + " channels on port " + std::to_string(port_) + ")";
    }
}

DigitalState AdvantechDIO::read_state() const {
    
    return state_;
}

void AdvantechDIO::write_state( DigitalState& state ) {
    
    if (state.nchannels()!=nchannels_) {
        throw DigitalDeviceError("Incompatible number of channels.");
    }
    
    std::vector<uint8_t> port_values( nports_, 0 );
    uint32_t channel_idx = 0;
    
    for ( uint32_t p = 0; p<nports_; ++p ) {
        for (uint32_t c = 0; c<CHANNELS_PER_PORT; ++c) {
            if (state[channel_idx]) {
                port_values[p] |= (1<<c);
            }
            ++channel_idx;
        }
    }
    
    if (port_<0) {
        for (unsigned int p=0; p<nports_; ++p) {
            // insert short sleep to work around time-out errors in advantech DIO
            custom_sleep_for( delay_ );
            write( p, port_values[p] );
        }
    } else {
        // insert short sleep to work around time-out errors in advantech DIO
        custom_sleep_for( delay_ );
        write( port_, port_values[0] );
    }
    
    state_ = state;
    
}

void AdvantechDIO::read( std::vector<uint8_t> & values ) const {
    
    values.assign( nports_, 0 );
    auto ret = device_->Read(0, nports_, values.data());
    if (ret!=0) { throw DigitalDeviceError( "Read", advantech_error_to_string(ret) ); }
}

void AdvantechDIO::read( uint32_t port, uint8_t & value ) const {
    
    if (port>=nmaxports_) { throw DigitalDeviceError("Invalid port number."); }
    
    auto ret = device_->Read(port, value);
    if (ret!=0) { throw DigitalDeviceError( "Read", advantech_error_to_string(ret) ); }
}

void AdvantechDIO::write( uint32_t port, uint8_t value ) const {
    
    if (port>=nmaxports_) { throw DigitalDeviceError("Invalid port number."); }
    
    auto ret = device_->Write(port, value);
    if (ret!=0) { throw DigitalDeviceError( "Write", advantech_error_to_string(ret) ); }
}

void AdvantechDIO::write( std::vector<uint8_t> & values ) const {
    
    if (values.size() != nmaxports_) { throw DigitalDeviceError("Invalid vector size."); }
    
    auto ret = device_->Write( 0, nports_, values.data() );
    if (ret!=0) { throw DigitalDeviceError( "Write", advantech_error_to_string(ret) ); }
}

std::string advantech_error_to_string( Automation::BDaq::ErrorCode_DIO error_code ) {
    
    switch (error_code) {
        case (Automation::BDaq::ErrorCode_DIO::Success): return "The operation is completed successfully.";
        case (Automation::BDaq::ErrorCode_DIO::WarningIntrNotAvailable) : return "The interrupt resource is not available.";
        case (Automation::BDaq::ErrorCode_DIO::WarningParamOutOfRange) : return "The parameter is out of the range.";
        case (Automation::BDaq::ErrorCode_DIO::WarningPropValueOutOfRange) : return "The property value is out of range.";
        case (Automation::BDaq::ErrorCode_DIO::WarningPropValueNotSpted) : return "The property value is not supported.";
        case (Automation::BDaq::ErrorCode_DIO::WarningPropValueConflict) : return "The property value conflicts with the current state.";
        case (Automation::BDaq::ErrorCode_DIO::WarningVrgOfGroupNotSame) : return "The value range of all channels in a group should be same, such as 4~20mA of PCI-1724.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorHandleNotValid) : return "The handle is NULL or its type does not match the required operation.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorParamOutOfRange) : return "The parameter value is out of range.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorParamNotSpted) : return "The parameter value is not supported.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorParamFmtUnexpted) : return "The parameter value format is not the expected.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorMemoryNotEnough) : return "Not enough memory is available to complete the operation.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorBufferIsNull) : return "The data buffer is null.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorBufferTooSmall) : return "The data buffer is too small for the operation.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorDataLenExceedLimit) : return "The data length exceeded the limitation.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorFuncNotSpted) : return "The required function is not supported.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorEventNotSpted) : return "The required event is not supported.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorPropNotSpted) : return "The required property is not supported.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorPropReadOnly) : return "The required property is read-only.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorPropValueConflict) : return "The specified property value conflicts with the current state.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorPropValueOutOfRange) : return "The specified property value is out of range.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorPropValueNotSpted) : return "The specified property value is not supported.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorPrivilegeNotHeld) : return "The handle hasn't own the privilege of the operation the user wanted.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorPrivilegeNotAvailable) : return "The required privilege is not available because someone else had own it.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorDriverNotFound) : return "The driver of specified device was not found.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorDriverVerMismatch) : return "The driver version of the specified device mismatched.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorDriverCountExceedLimit) : return "The loaded driver count exceeded the limitation.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorDeviceNotOpened) : return "The device is not opened.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorDeviceNotExist) : return "The required device does not exist.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorDeviceUnrecognized) : return "The required device is unrecognized by driver.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorConfigDataLost) : return "The configuration data of the specified device is lost or unavailable.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorFuncNotInited) : return "The function is not initialized and can't be started.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorFuncBusy) : return "The function is busy.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorIntrNotAvailable) : return "The interrupt resource is not available.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorDmaNotAvailable) : return "The DMA channel is not available.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorDeviceIoTimeOut) : return "Time out when reading/writing the device.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorSignatureNotMatch) : return "The given signature does not match with the device current one.";
        case (Automation::BDaq::ErrorCode_DIO::ErrorUndefined) : return "Undefined error.";
        default:
            throw std::runtime_error("No such error code is known.");
    }
}

