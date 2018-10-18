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

#include "digitaloutput.hpp"

#include "g3log/src/g2log.hpp"
#include "dio/dummydio.hpp"
#include "dio/advantechdio.hpp"

#include <iostream>

void DigitalOutput::Configure(const YAML::Node& node, const GlobalContext& context) {
    
    default_enabled_ =
        node["enabled"].as<decltype(default_enabled_)>( DEFAULT_ENABLED );
    
    default_lockout_period_ms_ =
        node["lockout_period"].as<decltype(default_lockout_period_ms_)>(
        DEFAULT_LOCKOUT_PERIOD_MS );
    
    if ( default_lockout_period_ms_ <= 0 ) {
        LOG(INFO) << name() << ". No lockout period set.";
    } else {
        LOG(INFO) << name() << ". Max stimulation frequency set to " <<
            1e3 / static_cast<double>( default_lockout_period_ms_ ) << " Hz.";
    }
    
    save_stim_events_ = node["enable_saving"].as<decltype(save_stim_events_)> (
        DEFAULT_SAVE_STIM_EVENTS );
    
    unsigned int pulse_width =
        node["pulse_width"].as<unsigned int>( DEFAULT_PULSE_WIDTH_MICROSEC );
    
    default_disable_delays_ = node["remove_stim_delays"].as<decltype(default_disable_delays_)>(
        DEFAULT_DISABLE_DELAYS );
    
    if (!node["device"] || !node["device"].IsMap() || !node["device"]["type"]) {
        throw ProcessingConfigureError(
            "No valid digital output device specified.", name() );
    }
    
    std::string device_type = node["device"]["type"].as<std::string>( );
    if (device_type=="dummy") {
        std::uint32_t nchannels =
            node["device"]["nchannels"].as<std::uint32_t>( DEFAULT_DUMMY_NCHANNELS );
        device_ = std::unique_ptr<DigitalDevice>( new DummyDIO( nchannels ) );
    } else if (device_type=="advantech") {
        int port = node["device"]["port"].as<int>( DEFAULT_ADVANTECH_PORT );
        std::uint64_t delay =
            node["device"]["delay"].as<std::uint64_t>( DEFAULT_ADVANTECH_DELAY );
        device_ =
            std::unique_ptr<DigitalDevice>( new AdvantechDIO( port, delay ) );
    } else {
        throw ProcessingConfigureError(
            "No valid digital output device specified.", name() );
    }
    
    LOG(INFO) << "Opened digital output device " << device_->description() << ".";
    
    ProtocolYAMLMap p;
    if (node["protocols"]) {
        p = node["protocols"].as<ProtocolYAMLMap>( );
    }
    
    bool delay_set;
    for (auto const & it : p) {
        delay_set = false;
        protocols_[it.first] = std::unique_ptr<DigitalOutputProtocol>(
            new DigitalOutputProtocol( device_->nchannels(), pulse_width ) );
	protocols_[it.first]->set_delay( 0 );
	
        for (auto const & it2 : it.second ) {
            if (it2.first == "toggle") {
                protocols_[it.first]->set_mode( it2.second, DigitalOutputMode::TOGGLE );
            } else if (it2.first == "high") {
                protocols_[it.first]->set_mode( it2.second, DigitalOutputMode::HIGH);
            } else if (it2.first == "low") {
                protocols_[it.first]->set_mode( it2.second, DigitalOutputMode::LOW );
            } else if (it2.first == "pulse") {
                protocols_[it.first]->set_mode( it2.second, DigitalOutputMode::PULSE );
                LOG(DEBUG) << name() << ". Pulsing protocol.";
            } else if (it2.first == "delay") {
		protocols_[it.first]->set_delay( it2.second );
                delay_set = true;
                LOG(UPDATE) << name() << ". Pulsing protocol with delay.";
            }
        }
	
        if ( protocols_[it.first]->delay() == 0 or disable_delays_) {
            LOG(INFO) << name() << ". Protocol for event " << it.first <<
                " will be executed without additional delay.";
        } else {
            LOG(INFO) << name() << ". Protocol for event " << it.first <<
                " will be executed after " << protocols_[it.first]->delay() << " ms.";
        }
    }
    
    LOG(INFO) << name() << ". There are " << protocols_.size()
        << " configured output protocols.";
    
    print_protocol_execution_updates_ =
        node["print_protocol_execution_updates"].as<decltype(print_protocol_execution_updates_)>( true );
    
    roundtrip_latency_test_ = node["roundtrip_latency_test"].as<decltype(roundtrip_latency_test_)>(
        DEFAULT_LATENCY_TEST );
}

void DigitalOutput::CreatePorts() {
    
    data_in_port_ = create_input_port(
        "events",
        EventDataType(),
        PortInPolicy( SlotRange(1, 4) ) );
    
    enabled_state_ = create_readable_shared_state(
        "enabled",
        default_enabled_,
        Permission::READ,
        Permission::WRITE);
    
    lockout_period_ms_ = create_readable_shared_state(
        "lockout_period",
        default_lockout_period_ms_,
        Permission::READ,
        Permission::WRITE);
    
    disable_delays_ = create_readable_shared_state(
        "disable_delays",
        default_disable_delays_,
        Permission::READ,
        Permission::WRITE);
}

void DigitalOutput::Preprocess( ProcessingContext& context ) {
    
    // reset counters and logs
    nreceived_events_ = 0;
    ntarget_events_ = 0;
    nprotocol_executions_ = 0;
    n_locked_out_events_ = 0;
    previous_TS_nostim_.assign( data_in_port_->number_of_slots(),
        std::numeric_limits<uint64_t>::min() );
    delta_TS_ms_.resize( data_in_port_->number_of_slots() );
     
    if ( context.test() and roundtrip_latency_test_ ) {
        prepare_latency_test( context );
    }
}
    
void DigitalOutput::Process( ProcessingContext& context ) {
     
    EventData* data_in = nullptr;
    uint64_t ts;
    SlotType s;
    
    std::string path = context.resolve_path( "run://", "run" );
    auto prefix = path + name();
    std::string filename;
    
    while (!context.terminated()) {

        for (s=0; s<data_in_port_->number_of_slots(); ++s) {
            
            if (!data_in_port_->slot(s)->RetrieveData( data_in )) {break;}
            ++ nreceived_events_;

            // select and execute protocol based on event name
            if (enabled_state_->get() && protocols_.count( data_in->event() ) > 0 ) {

                ++ntarget_events_;

                if ( not to_lock_out( data_in->hardware_timestamp(), s ) ) {

                    try {

                        if ( context.test() and roundtrip_latency_test_ ) {
                            test_source_timestamps_[nprotocol_executions_] = Clock::now();
                        }

                        protocols_[ data_in->event() ]->execute( *device_, disable_delays_->get() );
                        ++ nprotocol_executions_;
                        LOG_IF(UPDATE, print_protocol_execution_updates_) << name()
                            << ". Protocol executed for " << data_in->event() << " event.";
                    } catch ( DigitalDeviceError & e ) {
                        LOG(WARNING) << ". Could not execute protocol for event ("
                            << data_in->event() << ") : " << e.what();
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
            data_in_port_->slot(s)->ReleaseData();
        }
    }
 }

void DigitalOutput::Postprocess( ProcessingContext& context ) {
    
    LOG(INFO) << name() << ". Received " << nreceived_events_ <<
        " events, of which " << ntarget_events_ <<
        " were targets. Successfully executed stimulation protocol " <<
        nprotocol_executions_ << " times out of " << ntarget_events_ << ". " <<
        n_locked_out_events_ << " executions of the stimulation protocol were locked out.";
    
    if ( context.test() and roundtrip_latency_test_ ) {
        save_source_timestamps_to_disk( nprotocol_executions_ );
    } 
}

bool DigitalOutput::to_lock_out( const uint64_t current_timestamp, SlotType s  ) {
    
    if ( current_timestamp < previous_TS_nostim_[s] ) {
        LOG(UPDATE) << name() << "current ts: " << current_timestamp <<
            ". previous TS: " << previous_TS_nostim_[s];
        
        throw ProcessingError( "Non-sequential stimulation event timestamp.", name() );
    }
    delta_TS_ms_[s] = (current_timestamp - previous_TS_nostim_[s]) / 1000;

    if ( delta_TS_ms_[s] <= lockout_period_ms_->get() ) { return true;}
    
    previous_TS_nostim_[s] = current_timestamp;
    return false;
}
