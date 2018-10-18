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
 * EventConverter: convert incoming EventData into another event
 * 
 * input ports:
 * events <EventData> (1 slot1)
 * 
 * output ports:
 * events <IData> (1 slot)
 *
 * 
 * options:
 * event_name <string> - name for generating the delayed event
 * replace <bool> - if True output events will be replaced by "event_name" events,
 * if False output events will have "event_name" appended to their original name
 */

#ifndef EVENT_CONVERTER_HPP
#define	EVENT_CONVERTER_HPP

#include "../graph/iprocessor.hpp"
#include "../data/eventdata.hpp"

class EventConverter : public IProcessor {
    
public:
    virtual void Configure( const YAML::Node& node, const GlobalContext& context) override;
    virtual void CreatePorts() override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;

protected:
    PortIn<EventDataType>* data_in_port_;
    PortOut<EventDataType>* data_out_port_;

    std::string event_name_;
    bool replace_;

public:
    const std::string DEFAULT_EVENT_NAME = "stimulation";
    const decltype(replace_) DEFAULT_REPLACE = true;
};

#endif	// eventconverter.hpp

