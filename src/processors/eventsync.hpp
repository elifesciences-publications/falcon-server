/* EventSync: synchronizes on the occurrence of a target event on all
 * its input slots, before emitting the same target event
 * 
 * input ports:
 * events <EventData> (1-256 slots)
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
 * target_event <string> - target event
 * 
 */

#ifndef EVENTSYNC_HPP
#define EVENTSYNC_HPP

#include "../graph/iprocessor.hpp"
#include "../data/eventdata.hpp"
#include "utilities/general.hpp"
#include "utilities/time.hpp"

class EventSync : public IProcessor {
    
public:
    virtual void Configure( const YAML::Node& node, const GlobalContext& context ) override;
    virtual void CreatePorts() override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;

protected:
    void reset_timestamps(TimestampRegister timestamp_reg);
    void update_latest_ts(EventData* data_in);
    void read_target_event( const YAML::Node& node );
    void log_and_reset_counters( PortIn<EventDataType>* in_port, EventCounter& counter );
    
protected:
    PortIn<EventDataType>* data_in_port_;
    PortOut<EventDataType>* data_out_port_;
    EventData target_event_;

    EventCounter event_counter_;
    uint64_t n_events_synced_;
    
    TimestampRegister timestamps_;
};

#endif // eventsync.hpp
