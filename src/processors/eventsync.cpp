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

#include "eventsync.hpp"
#include "g3log/src/g2log.hpp"

void EventSync::Configure(const YAML::Node& node, const GlobalContext& context ) {
    
    read_target_event( node );   
}

void EventSync::CreatePorts() {
    
    data_in_port_ = create_input_port(
        "events",
        EventDataType(),
        PortInPolicy( SlotRange(1, 256) ) );
    
    data_out_port_ = create_output_port(
        "events",
        EventDataType( target_event_.event() ),
        PortOutPolicy( SlotRange(1) ) );
}

void EventSync::Process( ProcessingContext& context ) {
    
    EventData* data_in = nullptr;
    EventData* data_out = nullptr;
    
    uint64_t target_events_counter = 0;
    
    timestamps_.reset();
    
    while ( !context.terminated() ) {

        for ( SlotType s=0; s < data_in_port_->number_of_slots(); ++s ) {

            if (!data_in_port_->slot(s)->RetrieveData(data_in)) {break;}
            ++ event_counter_.all_received;
            
            if (*data_in == target_event_) {  
                ++ target_events_counter;
                ++ event_counter_.target;
                update_latest_ts( data_in );
            } else {
                ++ event_counter_.non_target;
            }
            
            data_in_port_->slot(s)->ReleaseData();
            
        }
          
        if ( target_events_counter == data_in_port_->number_of_slots() ) {
            
            data_out = data_out_port_->slot(0)->ClaimData(false);
            
            data_out->set_source_timestamp( );
            data_out->set_hardware_timestamp( timestamps_.hw );
            
            data_out->set_event( target_event_ );
            data_out_port_->slot(0)->PublishData();
            target_events_counter = 0;
            ++ n_events_synced_;
            
            timestamps_.reset();
    
        }   
    }
}

void EventSync::Postprocess( ProcessingContext& context ) {
    
    log_and_reset_counters( data_in_port_, event_counter_ );
    
    LOG(INFO) << name() << ". " << n_events_synced_ << " events synced.";
    n_events_synced_ = 0;
}
    
void EventSync::update_latest_ts(EventData* data_in) {
        
    if ( data_in->source_timestamp() > timestamps_.source ) {
        timestamps_.source = data_in->source_timestamp();
    }
    if ( data_in->hardware_timestamp() > timestamps_.hw  ) {
        timestamps_.hw = data_in->hardware_timestamp();
    } 
}

void EventSync::log_and_reset_counters( PortIn<EventDataType>* in_port, 
    EventCounter& counter ) {
    
    auto msg = ". '" + in_port->name() + "' counters.\n\t\t\t\t" +
        std::to_string( counter.all_received ) + " events received.\n\t\t\t\t" +
        std::to_string( counter.target ) + " target events received.\n\t\t\t\t" +
        std::to_string( counter.non_target ) + " non-target events received";
    if ( counter.consistent_counters() ) {
        LOG(INFO) << name() << msg << ". Counters are consistent.";
    } else {
        LOG(WARNING) << name() << ". Counters are inconsistent.";
    }
    
    counter.reset();
}

void EventSync::read_target_event( const YAML::Node& node ) {
    
    target_event_ = EventData( node["target_event"].as<std::string>( DEFAULT_EVENT ) );
}
