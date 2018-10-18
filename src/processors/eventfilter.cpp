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

#include "eventfilter.hpp"

#include "g3log/src/g2log.hpp"

#include <chrono>
#include <thread>
#include <algorithm>

void EventFilter::Configure(const YAML::Node& node, const GlobalContext& context) {
    
    read_target_event( node );
    
    blockout_time_ms_ = node["blockout_time_ms"].as<decltype(blockout_time_ms_)>(
        DEFAULT_BLOCKOUT_TIME_MS );
    
    synch_time_ms_ = node["synch_time_ms"].as<decltype(synch_time_ms_)>(
        DEFAULT_SYNCH_TIME_MS );
    
    time_in_ms_ = node["time_in_ms"].as<decltype(time_in_ms_)>( DEFAULT_TIME_IN_MS );
    
    discard_warnings_ = node["discard_warnings"].as<decltype(discard_warnings_)>
        ( DEFAULT_WARNINGS_DISCARDED );
    
    auto detection_criterion = node["detection_criterion"].as<std::string>( "any" );
    if ( detection_criterion == "any" ) {
        detections_to_criterion_ = 1;
    } else if ( detection_criterion == "all" ) {
        detections_to_criterion_ = ALL; // 'all' criteria will be configured in Prepare
    } else {
        detections_to_criterion_ =
            node["detection_criterion"].as<decltype(detections_to_criterion_)>( 1 );
        // check is in Prepare
    }
    
}

void EventFilter::CreatePorts() {
    
    data_in_port_ = create_input_port(
        "events",
        EventDataType(),
        PortInPolicy( SlotRange(1, 256), false, 0 ) );
    
    block_in_port_ = create_input_port(
        "blocking_events",
        EventDataType(),
        PortInPolicy( SlotRange(1, 256), false, 0 ) );

    data_out_port_ = create_output_port(
        "events",
        EventDataType( target_event_.event() ),
        PortOutPolicy( SlotRange(1) ) );
    
}

void EventFilter::Prepare( GlobalContext& context ) {
    
    auto nslots = data_in_port_->number_of_slots();
    
    // complete Configure
    if ( detections_to_criterion_ == ALL ) {
        detections_to_criterion_ = nslots;
    }
    
    // check detections_to_criterion value
    if ( detections_to_criterion_ < 1 or detections_to_criterion_ > nslots ) {
        auto err_msg = std::string("Invalid number of detections to criterion.")
            + "It must be a number between 1 and " + std::to_string(nslots) + ".";
        throw ProcessingPrepareError( err_msg, name() );
    }
    LOG(INFO) << name() << ". Criterion for triggering an event is set to " <<
        detections_to_criterion_ << " events.";
    LOG(INFO) << name() << ". Criterion for triggering a blocking event is set to 1 event.";
}

void EventFilter::Preprocess( ProcessingContext& context ) {

    // init gate_close_time, but make sure the first event won't be excluded
    // if no blocking event will be received
    gate_close_time_ = Clock::now();
    if ( blockout_time_ms_ > 0 ) {
        std::this_thread::sleep_for( std::chrono::milliseconds(
            static_cast<int>( blockout_time_ms_ ) ) );
    }
}

void EventFilter::Process(ProcessingContext& context) {
    
    EventData* data_out = nullptr;
    
    bool alive = false;
    bool detection_criterion = false;
    bool event_received = false;
    decltype(detections_to_criterion_) counter_to_detection = 0;
    bool detection_block = false;
    std::size_t slot_last = 0;
    
    bool gate_just_closed = false;
    TimePoint t_detection;
    std::chrono::duration<double, std::milli> duration_ms;

    std::vector<TimePoint> arrival_times_per_slot_events( data_in_port_->number_of_slots(),
        std::numeric_limits<TimePoint>::min() );
    std::vector<uint64_t> arrival_hwTS_per_slot_events(
        data_in_port_->number_of_slots(), 0 );
    
    // not used but needs to be passed
    std::vector<TimePoint> arrival_times_per_slot_blocking_events(
        block_in_port_->number_of_slots(), std::numeric_limits<TimePoint>::min() ); 
    std::vector<uint64_t> arrival_hwTS_per_slot_blockingevents(
        block_in_port_->number_of_slots(), 0 );
    
    // t >= (t_last - time_in_ms ) for t in log_n_slots-> how many slots meet criterion?
    // t_last - t <= time_in_ms

    while ( !context.terminated() ) {
        
        if ( not detection_criterion ) {
            
            // read input port for triggering events
            std::tie( alive, event_received, slot_last ) = is_there_target(
                data_in_port_, event_counter_, arrival_times_per_slot_events,
                arrival_hwTS_per_slot_events );
            if ( not alive ) {break;}
            if ( event_received ) {
                counter_to_detection = 0;
                for ( auto t: arrival_times_per_slot_events ) {
                    if ( time_between( arrival_times_per_slot_events[slot_last], t )
                    < time_in_ms_ ) {
                        ++ counter_to_detection;
                    }
                }
                detection_criterion = ( counter_to_detection >= detections_to_criterion_ );
                LOG(DEBUG) << name() << ". Detection criterion met.";
            }
            
            // read input port for blocking events
            std::tie( alive, detection_block, std::ignore ) = is_there_target(
                block_in_port_, blocking_events_counter_,
                arrival_times_per_slot_blocking_events,
                arrival_hwTS_per_slot_blockingevents );
            if ( not alive ) {break;}
            if ( detection_block ) {
                gate_close_time_ = Clock::now();
                detection_block = false;
            }
        }

        if ( detection_criterion ) { // check again as flag might have just changed

            t_detection = Clock::now();
            // check if gate is closed
            if ( time_since( gate_close_time_ ) <= blockout_time_ms_ ) {
                ++ n_blocked_events_;
                detection_criterion = false;
                LOG( UPDATE ) << name() << ". Target event " << target_event_.event()
                    << " was filtered out.";
            } else {
                // if open, before streaming the event on the output,
                // check if blocking event is coming soon after the target event
                // is received on the "events" port with this dedicated read loop
                
                // read incoming blocking events for synch_time_ms_
                while ( time_since( t_detection ) < synch_time_ms_ and detection_criterion ) {
                    std::tie( alive, gate_just_closed, std::ignore ) = is_there_target(
                        block_in_port_, blocking_events_counter_,
                        arrival_times_per_slot_blocking_events,
                        arrival_hwTS_per_slot_blockingevents );
                    if ( not alive ) {break;} // exit inner while loop

                    if ( gate_just_closed ) {
                        ++ n_blocked_events_;
                        LOG( UPDATE ) << name() << ". Target event " << target_event_.event()
                            << " was filtered out (blocking event arrived after target).";
                        detection_criterion = false;
                        gate_close_time_ = Clock::now();
                    }
                }
                if ( not alive ) {break;} // break outer (context) while loop 
                
                if ( not gate_just_closed ) { // no post detection block
                    
                    // finally send event
                    data_out = data_out_port_->slot(0)->ClaimData( false );
                    data_out->set_hardware_timestamp(
                        arrival_hwTS_per_slot_events[slot_last] );
                    data_out->set_source_timestamp();
                    data_out_port_->slot(0)->PublishData();

                    detection_criterion = false;  
                }
            }
        }
        
    }  
}

void EventFilter::Postprocess( ProcessingContext& context ) {
    
    log_and_reset_counters( data_in_port_, event_counter_ );
    log_and_reset_counters( block_in_port_, blocking_events_counter_ );
    
    LOG(INFO) << name() << ". Streamed " << data_out_port_->slot(0)->nitems_produced()
        << " target events.";
    
    LOG(INFO) << name() << ". " << n_blocked_events_ << " target events were blocked.";    
    n_blocked_events_ = 0;
}

std::tuple<bool, bool, std::size_t> EventFilter::is_there_target(
    PortIn<EventDataType>* input_port, EventCounter& event_counter,
    std::vector<TimePoint>& arrival_times, std::vector<uint64_t>& arrival_timestamps ) {
    
    std::vector<EventData*> data_in;
    std::size_t slot_index = std::numeric_limits<std::size_t>::max();
    bool target_received = false;
    
    for ( decltype(input_port->number_of_slots()) s=0; s<input_port->number_of_slots(); ++s ) {

        // check if processor is still alive
        if ( !input_port->slot(s)->RetrieveDataAll( data_in ) ) {
            return std::make_tuple( false, false, NULL_TIMESTAMP );
        }
        
        // check if there is data on any of the slot
        auto nread = input_port->slot(s)->status_read();
        if ( nread == 0 ) {
            input_port->slot(s)->ReleaseData();
            continue;
        }
        if ( nread>1 and not discard_warnings_ ) {
            std::string events_list = data_in[1]->event();
            for ( auto el=data_in.begin()+1; el!=data_in.end()-1; ++el ) {
                events_list += ( ", " + (*el)->event() );
            }
            LOG(WARNING) << name() << ". " << nread-1 <<
                " events on port " << input_port->name() << "( " << events_list
                << " ) were discarded.";
        }
        ++ event_counter.all_received;

        // if there's data, check if it is a target event
        if ( *data_in.back() == target_event_ ) {
            
            LOG(DEBUG) << name() << ". Received target event " <<
                target_event_.event() << " on port " << input_port->name() << " slot " << s;
            
            ++ event_counter.target;
            arrival_times[s] = Clock::now();
            arrival_timestamps[s] = data_in.back()->hardware_timestamp();
            target_received = true;
            slot_index = s;
            
        } else { // non-target event received
            
            LOG( DEBUG ) << name() << ". Received non-target event: " <<
                data_in.back()->event() << " on port " << input_port->name() << " slot " << s;
            ++ event_counter.non_target;
            
        }
        input_port->slot(s)->ReleaseData();
    }
    
    // all slots read, no data on any of them
    return std::make_tuple( true, target_received, slot_index );
}
