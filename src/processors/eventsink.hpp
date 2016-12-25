/* EventSink: takes an EventData stream and logs the arrival of a target event
 * 
 * input ports:
 * events <EventData> (1 slot)
 *
 * output ports:
 * none
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

#ifndef EVENTSINK_HPP
#define EVENTSINK_HPP

#include "../graph/iprocessor.hpp"
#include "../data/eventdata.hpp"
#include "utilities/general.hpp"

class EventSink : public IProcessor
{
public:
    virtual void Configure( const YAML::Node& node, const GlobalContext& context) override;
    virtual void CreatePorts() override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override; 

protected:
    PortIn<EventDataType>* event_port_;
    EventData target_event_;
    
    EventCounter event_counter_;
};

#endif //eventsink.hpp
