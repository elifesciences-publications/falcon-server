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

/* 
 * EventFilter: processes neural data
 * 
 * input ports:
 * events <EventData> ( 1-256 slots)
 * blocking_events <EventData> ( 1-256 slots)
 * 
 * output ports:
 * events <EventData> (1 slot)
 * 
 * exposed states:
 * none
 *
 * exposed methods:
 * none
 * 
 * options:
 * target_event <string> - target event to be filtered out
 * blockout_time_ms <double> - time during which target events are filtered out
 * synch_time_ms <double> - time used to check if any blocking target event is present
 * after a target event has been received 
 * detection_criterion <string OR unsigned int> - string or number to determine 
 * the criterion for a triggering detection; acceptable string values: 'any', 'all'
 * acceptable integer values: any value between 1 (equivalent to 'any') and the 
 * number of input slots 
 * discard_warnings <bool> - if true, warnings about discarded events will not
 * be generated
 */

#ifndef EVENT_FILTER_HPP
#define	EVENT_FILTER_HPP

#include "eventsync.hpp"
#include "utilities/time.hpp"

#include <tuple>
#include <chrono>


class EventFilter : public EventSync {
    
public:
    virtual void Configure( const YAML::Node& node, const GlobalContext& context) override;
    virtual void CreatePorts() override;
    virtual void Prepare( GlobalContext& context ) override;
    virtual void Preprocess( ProcessingContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;   

protected:
    // return the alve status of the processor, the flag of found target and
    // the timestamp of the target event detected after a read on the input port
    std::tuple<bool, bool, std::size_t> is_there_target(
        PortIn<EventDataType>* input_port,
        EventCounter& event_counter,
        std::vector<TimePoint>& arrival_times,
        std::vector<uint64_t>& arrival_timestamps  );
    
protected:
    PortIn<EventDataType>* block_in_port_;

    double blockout_time_ms_;
    double synch_time_ms_;
    double time_in_ms_;
    SlotType detections_to_criterion_;
    bool discard_warnings_;
    
    unsigned int n_blocked_events_;
    EventCounter blocking_events_counter_;
    
    TimePoint gate_close_time_;

protected:
    std::chrono::duration<double, std::milli> duration_ms_;
    
    // return the time in milliseconds past from two given time points
    inline double time_between( TimePoint t2, TimePoint t1 ) {
        
        duration_ms_ = t2 - t1;
        return duration_ms_.count();
    }
    
    // return the time in milliseconds past from a given time point
    inline double time_since( TimePoint t ) {
        
        return time_between( Clock::now(), t );
    }
    
    
public:
    const decltype(blockout_time_ms_) DEFAULT_BLOCKOUT_TIME_MS = 10.0;
    const decltype(synch_time_ms_) DEFAULT_SYNCH_TIME_MS = 1.5;
    const decltype(time_in_ms_) DEFAULT_TIME_IN_MS = 3.5;
    const decltype(discard_warnings_) DEFAULT_WARNINGS_DISCARDED = false;
    
protected:
    const uint64_t NULL_TIMESTAMP = std::numeric_limits<uint64_t>::max();
    const decltype(detections_to_criterion_) ALL =
        std::numeric_limits<decltype(detections_to_criterion_)>::max();
};

#endif	// eventfilter.hpp

