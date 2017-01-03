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

#ifndef PROCESSORGRAPH_H
#define PROCESSORGRAPH_H

#include <exception>
#include <cstring>
#include <memory>
#include <map>
#include <utility>

#include "yaml-cpp/yaml.h"
#include "g3log/src/g2log.hpp"

#include "processorengine.hpp"
#include "graphexceptions.hpp"
#include "connectionparser.hpp"
#include "runinfo.hpp"

namespace graph {

enum class GraphState { NOGRAPH, CONSTRUCTING, PREPARING, READY, STARTING, PROCESSING, STOPPING, ERROR };

class ProcessorGraph
{
public:
    ProcessorGraph( GlobalContext& context ) : global_context_(context), terminate_signal_(false) {}
    
    bool terminated() { return terminate_signal_.load(); }
    
    bool done() {
        // graph is done, if it has terminated or PROCESSING and no processor is running
        if ( state_ != GraphState::PROCESSING ) { return false; }
        
        if (terminated()) { LOG(DEBUG) << "done: terminated=true."; return true; }
        
        return !any_processor_running();
        
    }
    
    bool all_processors_running() {
        for (auto & it : engines_ ) {
            if ( !it.second.second->running() ) {
                return false;
            }
        }
        return true;
    }
    bool any_processor_running() {
        for (auto & it : engines_ ) {
            if ( it.second.second->running() ) {
                return true;
            }
        }
        return false;
    }
    
    void Build( const YAML::Node& node);
    void Destroy();
    void StartProcessing( std::string run_group_id, std::string run_id, std::string template_id, bool test_flag );
    void StopProcessing();
    void Update( YAML::Node& node );
    void Retrieve( YAML::Node& node );
    void Apply( YAML::Node& node );
    
    std::string ExportYAML();
    
    const GraphState state() const {return state_;};
    std::string state_string() const;
    void set_state(GraphState state) { state_ = state; LOG(STATE) << state_string(); }
    
    const ProcessorEngineMap& processors() const { return engines_; }
    const StreamConnections& connections() const { return connections_; }

    void LinkSharedStates( const YAML::Node& node );

private:
    YAML::Node yaml_;
    
    GlobalContext& global_context_;
    
    ProcessorEngineMap engines_;
    StreamConnections connections_;
    
    GraphState state_ = GraphState::NOGRAPH;
    
    std::unique_ptr< RunContext > run_context_;
    std::atomic<bool> terminate_signal_;
};


} //namespace graph

#endif // PROCESSORGRAPH_H
