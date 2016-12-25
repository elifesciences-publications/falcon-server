/* EventSource: generates an EventData stream by randomly emitting
 * events from a list of candidates at a predefined rate
 * 
 * input ports:
 * none
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
 * events <list of string> - list of events to emit
 * rate <double> - (approximate) event rate
 * 
 */
 
#ifndef EVENTSOURCE_HPP
#define EVENTSOURCE_HPP

#include "../graph/iprocessor.hpp"
#include "../data/eventdata.hpp"

class EventSource : public IProcessor {
    
public:
    virtual void Configure( const YAML::Node& node, const GlobalContext& context) override;
    virtual void CreatePorts() override;
    virtual void Process( ProcessingContext& context ) override;
    
protected:
    PortOut<EventDataType>* event_port_;
    
    std::vector<std::string> event_list_;
    double event_rate_;
    
public:
    const decltype(event_rate_) DEFAULT_EVENT_RATE = 1.0;
    const std::string DEFAULT_EVENT = "default_eventsource_event";
};

#endif // eventsource.hpp
