#include "eventsink.hpp"
#include <thread>

void EventSink::Configure( const YAML::Node& node, const GlobalContext& context) {
    
    target_event_ = EventData(node["target_event"].as<std::string>( "none" ));
}

void EventSink::CreatePorts() {
    
    event_port_ = create_input_port(
        "events",
        EventDataType(),
        PortInPolicy( SlotRange(1) ) );
}

void EventSink::Process( ProcessingContext& context ) {
    
    EventData *data;
    
    while (!context.terminated()) {
        
        if (!event_port_->slot(0)->RetrieveData(data)) {break;}
        
        ++ event_counter_.all_received;
        
        if (*data == target_event_) {
            ++ event_counter_.target;
            LOG(UPDATE) << name() << ": received target event " << data->event() << ".";
        } else {
            ++ event_counter_.non_target;
            LOG(UPDATE) << name() << ": skipped event " << data->event() << ".";
        }
        
        event_port_->slot(0)->ReleaseData();
        
    }
}

void EventSink::Postprocess( ProcessingContext& context ) {
    
    LOG(UPDATE) << name() << ". Received " << event_counter_.all_received
        << " events, of which " << event_counter_.target << " were targets.";
    if (event_counter_.consistent_counters()) {
        LOG(UPDATE) << name() << ". Counters are consistent.";
    }
    event_counter_.reset();
}