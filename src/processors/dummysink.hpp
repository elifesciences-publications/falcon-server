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

/* DummySink: takes an any data stream and eats it. Mainly used to show 
 * and test basic graph processing functionality.
 * 
 * input ports:
 * data <IData> (1 slot)
 *
 * output ports:
 * none
 *
 * exposed states:
 * tickle <bool> - logs message
 *
 * exposed methods:
 * kick - logs message
 *
 * options:
 * none
 * 
 */

#ifndef DUMMYSINK_H
#define DUMMYSINK_H

#include "../graph/iprocessor.hpp"

class DummySink : public IProcessor
{
public:
    virtual void CreatePorts() override;
    virtual void Process( ProcessingContext& context ) override;
    
    YAML::Node Kick( const YAML::Node & node );

protected:
    PortIn<AnyDataType>* data_port_;
    ReadableState<bool>* tickle_state_;
};

#endif
