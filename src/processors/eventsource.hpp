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
