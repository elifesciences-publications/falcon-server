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

#include "serialoutput.hpp"

#include "g3log/src/g2log.hpp"
#include "arduino-serial/arduino-serial-lib.h"
#include <iostream>
#include <fstream>

void SerialOutput::Configure(const YAML::Node& node, const GlobalContext& context) {
    
    default_enabled_ =
        node["enabled"].as<decltype(default_enabled_)>( DEFAULT_ENABLED );
    
    initial_lockout_period_ms_ =
        node["lockout_period_ms"].as<decltype(initial_lockout_period_ms_)>(
        DEFAULT_LOCKOUT_PERIOD_MS );
    
    if ( initial_lockout_period_ms_ <= 0 ) {
        LOG(INFO) << name() << ". No lockout period set.";
    } else {
        LOG(INFO) << name() << ". Max stimulation frequency set to " <<
            1e3 / static_cast<double>( initial_lockout_period_ms_ ) << " Hz.";
    }
    
    save_stim_events_ = node["enable_saving"].as<decltype(save_stim_events_)> (
        DEFAULT_SAVE_STIM_EVENTS );
    
    port_address_ = node["port_address"].as<std::string>( DEFAULT_PORT_ADDRESS );
    
    default_message_ = node["message"].as<decltype(default_message_)>( DEFAULT_MESSAGE );
    
    target_event_ = EventData( node["target_event"].as<std::string>( ) );
    
    baudrate_ = node["baudrate"].as<decltype(baudrate_)>( DEFAULT_BAUDRATE );
    print_transmission_updates_ =
        node["print_protocol_execution_updates"].as<decltype(print_transmission_updates_)>( true );
}

void SerialOutput::CreatePorts() {
    
    data_in_port_ = create_input_port(
        "events",
        EventDataType(),
        PortInPolicy( SlotRange(1) ) );
    
    enabled_state_ = create_readable_shared_state(
        "enabled",
        default_enabled_,
        Permission::READ,
        Permission::WRITE);
    
    lockout_period_ms_ = create_readable_shared_state(
        "lockout_period",
        initial_lockout_period_ms_,
        Permission::READ,
        Permission::WRITE);
    
    message_ = create_readable_shared_state(
        "message",
        default_message_,
        Permission::READ,
        Permission::WRITE);
}

void SerialOutput::Preprocess( ProcessingContext& context ) {
    
    // reset counters and logs
    nreceived_events_ = 0;
    ntarget_events_ = 0;
    nprotocol_executions_ = 0;
    n_locked_out_events_ = 0;
    previous_TS_nostim_ = std::numeric_limits<decltype(previous_TS_nostim_)>::min();
    
    if ( context.test() and roundtrip_latency_test_ ) {
        prepare_latency_test( context );
    }
    
    fd_ = serialport_init( port_address_.c_str(), baudrate_ );
    LOG(INFO) << "Serial port " << port_address_ << " opened.";
}
    
void SerialOutput::Process( ProcessingContext& context ) {
     
    EventData* data_in = nullptr;
    uint64_t ts;
    
    std::string path = context.resolve_path( "run://", "run" );
    auto prefix = path + name();
    std::string filename;
    char message;
    
    while (!context.terminated()) {

        if (!data_in_port_->slot(0)->RetrieveData( data_in )) {break;}
        ++ nreceived_events_;

        // select and execute protocol based on event name
        if (enabled_state_->get() && target_event_ == *data_in ) {

            ++ntarget_events_;

            if ( not to_lock_out( data_in->hardware_timestamp() ) ) {
                    
                if ( context.test() and roundtrip_latency_test_ ) {
                    test_source_timestamps_[nprotocol_executions_] = Clock::now();
                }

                message = message_->get();
                if ( (serialport_write( fd_, &message)) != 0 ) {
                    LOG(ERROR) <<  name() << ". Serial message " << message <<
                            " not delivered.";
                } else {
                    ++ nprotocol_executions_;
                    LOG_IF(UPDATE, print_transmission_updates_) << name()
                        << ". Message " << message << " transmitted serially for "
                            << data_in->event() << " event.";
                }
                
                if ( save_stim_events_ ) { //save stim events to disk
                    
                    filename = STIM_EVENT_S + data_in->event();
                    // filename will also be the key to the container of files
                    // check if this type of event has been saved before
                    if ( streams_.count( STIM_EVENT_S + data_in->event() ) == 0 ) {
                        create_file( prefix, filename );
                    }
                    ts = data_in->serial_number();
                    streams_[filename]->write(
                        reinterpret_cast<const char*>( &ts ), sizeof( decltype( ts ) ) );
                }

            } else {
                ++ n_locked_out_events_;
            }
        }

       data_in_port_->slot(0)->ReleaseData();
   }
    
 }

void SerialOutput::Postprocess( ProcessingContext& context ) {
    
    LOG(INFO) << name() << ". Received " << nreceived_events_ <<
        " events, of which " << ntarget_events_ <<
        " were targets. Successfully executed stimulation protocol " <<
        nprotocol_executions_ << " times out of " << ntarget_events_ << ". " <<
        n_locked_out_events_ << " executions of the stimulation protocol were locked out.";
    
    if ( context.test() and roundtrip_latency_test_ ) {
        save_source_timestamps_to_disk( nprotocol_executions_ );
    } 
    
    serialport_close( fd_ );
}

bool SerialOutput::to_lock_out( const uint64_t current_timestamp ) {
    
    if ( current_timestamp < previous_TS_nostim_ ) {
        throw ProcessingError( "Non-sequential stimulation event timestamp.", name() );
    }
    delta_TS_ms_ = (current_timestamp - previous_TS_nostim_) / 1000;

    if ( delta_TS_ms_ <= lockout_period_ms_->get() ) { return true;}
    
    previous_TS_nostim_ = current_timestamp;
    return false;
}
