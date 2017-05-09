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

/* SerialOutput: takes an EventData stream and sends data over serial port
 * when a target event is received
 * 
 * input ports:
 * events <EventData> (1 slot)
 *
 * output ports:
 * none
 *
 * exposed states:
 * enabled <bool> - enable/disable digital output
 *
 * exposed methods:
 * none
 *
 * options:
 * enabled <bool> - default for enabled state
 * port_address <string> - address serial port
 * message <string> - default message sent to the serial port
 * baudrate <int> - baudrate serial communication
 * target_event <string> - target event
 * lockout_period_ms <int> - initial lockout period [ms]  
 */

#ifndef SERIALOUTPUT_HPP
#define SERIALOUTPUT_HPP

#include "../graph/iprocessor.hpp"
#include "../data/eventdata.hpp"
#include "utilities/time.hpp"

#include <fstream>

class SerialOutput : public IProcessor {
    
public:
    virtual void Configure(const YAML::Node& node, const GlobalContext& context) override;
    virtual void CreatePorts() override;
    virtual void Preprocess( ProcessingContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;

protected:
    bool to_lock_out( const uint64_t current_timestamp );
    
protected:
    PortIn<EventDataType>* data_in_port_;
    
    bool default_enabled_;
    ReadableState<decltype(default_enabled_)>* enabled_state_;
    
    int initial_lockout_period_ms_;
    ReadableState<decltype(initial_lockout_period_ms_)>* lockout_period_ms_; 
    
    bool save_stim_events_;
    
    std::string message_;
    std::string port_address_;
    EventData target_event_;
    int baudrate_;
    int fd_;
    
    bool print_transmission_updates_;
    
    std::uint64_t nreceived_events_;
    decltype(nreceived_events_) ntarget_events_;
    decltype(nreceived_events_) nprotocol_executions_;
    decltype(nreceived_events_) n_locked_out_events_;
    
    uint64_t previous_TS_nostim_;
    decltype(initial_lockout_period_ms_) delta_TS_ms_;
    
public:
    const decltype(default_enabled_) DEFAULT_ENABLED = true;
    const decltype(initial_lockout_period_ms_) DEFAULT_LOCKOUT_PERIOD_MS = 50;
    const decltype(save_stim_events_) DEFAULT_SAVE_STIM_EVENTS = false;
    const decltype(baudrate_) DEFAULT_BAUDRATE = 9600;
    const std::string DEFAULT_MESSAGE = "1";
    const std::string DEFAULT_PORT_ADDRESS = "/dev/ttyACM0";
    
protected:
    const std::string STIM_EVENT_S = "stim_";
};

#endif // serialoutput.hpp
