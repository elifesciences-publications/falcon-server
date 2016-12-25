#include "eventsource.hpp"

#include <chrono>
#include <thread>
#include <random>


void EventSource::Configure( const YAML::Node& node, const GlobalContext& context) {
    
    std::vector<std::string> tmp_vec(1, DEFAULT_EVENT);
    event_list_ = node["events"].as<std::vector<std::string>>( tmp_vec );
    if ( event_list_ == tmp_vec) { // if defaults, check if a single string was entered
        auto tmp_event = node["events"].as<std::string>( DEFAULT_EVENT );
        event_list_.assign( 1, tmp_event );
    }
    for (auto& el: event_list_) {
        LOG(INFO) << name() << ". Event " << el << " configured for streaming.";
    }
    
    event_rate_ = node["rate"].as<double>( DEFAULT_EVENT_RATE );
    LOG(INFO) << name() << ". Event rate set to " << event_rate_ << " Hz.";
}

void EventSource::CreatePorts() {
    
    event_port_ = create_output_port(
        "events",
        EventDataType(),
        PortOutPolicy( SlotRange(1) ) );
}


void EventSource::Process( ProcessingContext& context ) {
    
    EventData *data = nullptr;
    
    if (event_list_.size()==0) { return; }
    
    std::default_random_engine generator;
    std::uniform_int_distribution<unsigned int> distribution(0, event_list_.size()-1);
    
    auto delay = std::chrono::milliseconds( static_cast<unsigned int>(1000.0/event_rate_) );
    
    while (!context.terminated()) {
        
        std::this_thread::sleep_for( delay );
        
        data = event_port_->slot(0)->ClaimData(false);
        
        data->set_source_timestamp();
        data->set_hardware_timestamp(
            static_cast<uint64_t>( data->time_since( context.run().start_time() ).count() ) );
        
        data->set_event( event_list_[distribution(generator)] );
        event_port_->slot(0)->PublishData();
        
    }
}
