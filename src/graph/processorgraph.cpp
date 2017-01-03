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

#include <iostream>
#include <exception>
#include <regex>

#include "processorgraph.hpp"
#include "iprocessor.hpp"
#include "g3log/src/g2log.hpp"
#include "utilities/general.hpp"

using namespace graph;

std::string graph_state_string( GraphState state ) {
    std::string s;
#define PROCESS_STATE(p) case(GraphState::p): s = #p; break;
    switch(state){
        PROCESS_STATE(NOGRAPH);
        PROCESS_STATE(CONSTRUCTING);
        PROCESS_STATE(PREPARING);
        PROCESS_STATE(READY);
        PROCESS_STATE(STARTING);
        PROCESS_STATE(PROCESSING);
        PROCESS_STATE(STOPPING);
        PROCESS_STATE(ERROR);
    }
#undef PROCESS_STATE
    return s;
}

std::vector<std::string> expandProcessorName( std::string s ) {
    
    std::vector<std::string> result;
    int startid, endid;
    
    // name# or name[#, #-#]
    std::regex re("(\\w+[a-zA-Z])((?:\\d+)|(?:\\([\\d,\\-]+\\)))?");
    std::smatch match;
    
    // remove all spaces
    s.erase( std::remove_if( s.begin(), s.end(), [](char x){ return std::isspace(x); } ), s.end() );
    
    // match regular expression
    if ( !std::regex_match(s, match, re) )
    { throw std::runtime_error("Invalid processor name: \"" + s + "\""); }
    
    //get base name
    if (!match[1].matched)
    { throw std::runtime_error("Invalid processor name (no base name): \"" + s + "\""); }
    std::string name = match[1].str();
    
    // parse part identifiers
    std::vector<int> identifiers;
    
    if (!match[2].matched) {
        result.push_back( name );
    } else {
        std::string piece = match[2].str();
        if (piece[0]=='(') {
            //match ID range vector
            //remove brackets and spaces
            piece.erase( std::remove_if( piece.begin(), piece.end(), [](char x){ return (x=='(' || x==')' || std::isspace(x)); } ), piece.end() );
            
            //split on comma
            auto id_pieces = split( piece, ',' );
            
            std::regex re_range("(\\d+)(?:\\-(\\d+))?");
            std::smatch match_range;
            
            //match start and end id of ranges
            for (const auto &q : id_pieces)
            {
                if (std::regex_match(q, match_range, re_range)) {
                    startid = stoi( match_range[1].str() );
                    if (match_range[2].matched) { endid = stoi( match_range[2].str() ); }
                    else { endid = startid; }
                    for (auto kk = startid; kk<=endid; kk++ )
                    { result.push_back( name + std::to_string(kk) ); }
                } else { throw std::runtime_error("Invalid processor name (invalid identifiers): \"" + s + "\""); }
            }
        } else {
            //try to convert to int
            try {
                result.push_back( name + std::to_string( stoi( piece ) ) );
            } catch ( std::invalid_argument &e )
            { throw std::runtime_error("Invalid processor name (invalid identifiers): \"" + s + "\""); }
        }
    }
    
    return result;
    
}

void ConstructProcessorEngines( const YAML::Node& node, ProcessorEngineMap& engines, const GlobalContext& context) {
    
    std::vector<std::string> processor_name_list;
    std::string processor_name;
    std::string processor_class;
    std::unique_ptr<ProcessorEngine> engine;

    // loop through all processors defined in YAML document
    for(YAML::const_iterator it=node.begin();it!=node.end();++it) {
        // expand processor name
        // e.g. name -> name, name1 -> name1, name(1-2, 4) -> name1, name2, name4
        processor_name_list = expandProcessorName( it->first.as<std::string>() );
        
        // get processor definition
        YAML::Node processor_node = it->second;
        
        if (processor_node["class"]) {
            processor_class = processor_node["class"].as<std::string>();
            
            // loop through expanded name list
            for (auto &name_it : processor_name_list ) {
                
                processor_name = name_it;
                
                // does processor already exist?
                auto it2 = engines.find( processor_name );
                
                if ( it2 == engines.end() ) { // no processor with this name known
                    
                    try {
                        engine.reset( new ProcessorEngine( processor_name, std::unique_ptr<IProcessor>( ProcessorFactory::instance().create( processor_class ) ) ) );
                    } catch ( factory::UnknownClass& e ) {
                        throw InvalidProcessorError( "Cannot create processor " + processor_name + " of unknown class " + processor_class + "." );
                    }
                    
                    engine->Configure( processor_node, context );
                    
                    engines[processor_name]= std::make_pair( processor_class, std::move(engine) );
                    
                    LOG(DEBUG) << "Constructed and configured " << processor_name << " (" << processor_class << ").";
                    
                } else if ( it2->second.first == processor_class ) { // processor with this name and class found
                    
                    it2->second.second->Configure( processor_node, context );
                    
                    LOG(DEBUG) << "Configured processor " << processor_name << " (" << processor_class << ")";
                    
                } else { // processor with this name, but different class found
                    throw InvalidProcessorError( "Processor " + processor_name + " of different class (" + it2->second.first + ") already exists." );
                }
            }
                
        } else {
            throw InvalidProcessorError( "No class specified for processor " + processor_name + ".");
        }
    }
}

void ParseConnectionRules( const YAML::Node& node, StreamConnections& connections ) {
    
    for(YAML::const_iterator it=node.begin();it!=node.end();++it) {
        expandConnectionRule( parseConnectionRule( it->as<std::string>() ), connections );
        LOG(DEBUG) << "Parsed connection rule " << it->as<std::string>();
    }
}

std::string ProcessorGraph::state_string() const {
    
    return graph_state_string( state_ );
}

void ProcessorGraph::LinkSharedStates( const YAML::Node& node ) {
    
    // node is a sequence
    // each item in the sequence should be a sequence of processor.state IDs to be linked:
    // states:
    //     - [processor.state, processor.state, ...]

    std::set<std::string> state_set;

    // loop through items in sequence:
    for(YAML::const_iterator link=node.begin();link!=node.end();++link) {
        // is item a sequence?
        if (!link->IsSequence()) {
            throw InvalidGraphError("Error parsing states: not a sequence.");
        }

        std::vector<std::pair<std::string, IState*>> states;

        // loop through items in sequence
        for(YAML::const_iterator linked_state=link->begin();linked_state!=link->end();
        ++linked_state) {  
            // parse procesor.state name
            std::vector<std::string> address = split(linked_state->as<std::string>(), '.');

            if (address.size()!=2) {
                throw InvalidGraphError("Error parsing state value address: " +
                    linked_state->as<std::string>() );
            }

            // expand state address
            auto p = expandProcessorName( address[0] );
            auto s = expandProcessorName( address[1] );
            std::vector<std::pair<std::string,std::string>> v;
            
            // combine
            for (auto & itp : p) {
                for (auto & its : s) {
                    v.push_back( std::make_pair( itp, its ) );
                }

            }

            // for each address:
            // already in the list?
            // processor exists?
            // get processor and create new connection
            for (auto & itv : v) {          

                if (state_set.count( itv.first + "." + itv.second )!=0) {
                    throw InvalidGraphError("Cannot link same state twice: " +
                        itv.first + "." + itv.second);
                }

                state_set.insert( itv.first + "." + itv.second );

                // find corresponding processor engine
                if (engines_.count( itv.first )==0) {
                    throw InvalidProcessorError(
                        "Error parsing state value address: no such processor \""
                            + itv.first + "\".");
                }

                ProcessorEngine* engine = engines_[itv.first].second.get();

                // look up state
                states.push_back( std::make_pair( linked_state->as<std::string>(),
                    engine->processor()->shared_state( itv.second )  ) );
            }

        }

        if (states.size()<2) {continue;} // nothing to link
        
        // check if all states are compatible with each other
        for (unsigned int m=0; m<states.size(); ++m) {
            for (unsigned int n=m+1; n<states.size(); ++n) {
                if (!states[m].second->IsCompatible( states[n].second ) ) {
                    throw InvalidGraphError("Cannot link incompatible states: \"" + states[m].first + "\" and \"" + states[n].first + "\"."  );
                }
            }
        }

        // determine master state, which will store the state value
        unsigned int master_state = 0;
        for (unsigned int m=0; m<states.size(); ++m) {
            if (states[m].second->permissions().self() != Permission::WRITE) {
                master_state = m;
                break;
            }
        }  

        states[master_state].second->SetMaster();

        // link all shared states to master state
        for (unsigned int m=0; m<states.size(); ++m) {
            if (m!=master_state) {
                states[m].second->Share( states[master_state].second );
                LOG(DEBUG) << "Successfully linked state " << states[m].first << " to master state " << states[master_state].first;
            }
        }
  
    }
}

void ProcessorGraph::Build( const YAML::Node& node ) {
    
    if (state_!=GraphState::NOGRAPH) {
        throw InvalidStateError("A graph has already been built. Destroy old graph first.");
    }
    
    if (!node["processors"] || !node["processors"].IsMap()) {
        throw InvalidGraphError("No processors found in graph definition.");
    }
    
    set_state( GraphState::CONSTRUCTING );
    
    try {
        
        ConstructProcessorEngines( node["processors"], engines_, global_context_ );
        LOG(INFO) << "Constructed and configured all processors";
        
        for (auto &it : this->engines_) {
            it.second.second->CreatePorts();
            LOG(DEBUG) << "Created ports for processor " << it.first;
        }
        LOG(INFO) << "All ports have been created.";
        
        if (node["connections"] && node["connections"].IsSequence()) {
            
            ParseConnectionRules( node["connections"], connections_ );
            LOG(INFO) << "Parsed all connection rules.";
            
            for ( auto &it : connections_ ) {
                it->Connect( this->engines_ );
                LOG(DEBUG) << "Established connection " << it->string();
            }
            LOG(INFO) << "All connections have established.";
        }
        
        if (node["states"] && node["states"].IsSequence()) {
            LinkSharedStates( node["states"] );
            LOG(INFO) << "Linked all shared states.";
        }
        
    } catch (...) {
        Destroy();
        throw;
    }
    
    
    try {
        // negiotiate connections
        for (auto &it : this->engines_) {
            it.second.second->NegotiateConnections();
            LOG(DEBUG) << "Negotiated data streams for processor " << it.first;
        }
        LOG(INFO) << "All data streams have been negotiated.";
        
        // build ringbuffers
        for (auto &it : this->engines_) {
            it.second.second->CreateRingBuffers();
            LOG(DEBUG) << "Constructed ring buffer for processor " << it.first;
        }
        
    } catch(...) {
        Destroy();
        throw;
    }
    
    set_state( GraphState::PREPARING );
    
    // prepare processors
    try {
        for (auto &it : this->engines_) {
            it.second.second->processor()->Prepare(global_context_);
            LOG(DEBUG) << "Successfully prepared processor " << it.first;
        }
        LOG(INFO) << "All processors have been prepared.";
    } catch ( ... ) {
        
        Destroy();
        throw;
    }
    
    yaml_ = node;
    
    LOG(INFO) << "Graph was successfully constructed.";
    
    set_state(GraphState::READY);
}

void ProcessorGraph::Destroy() {
    
    // can only destroy graph if state is CONSTRUCTING, PREPARING or READY
    if (state_==GraphState::PROCESSING || state_==GraphState::STARTING || state_==GraphState::STOPPING) {
        throw InvalidStateError("Cannot destroy graph while processing.");
    } else if (state_==GraphState::NOGRAPH) {
        // nothing to destroy
        return;
    }
    
    if (state_!=GraphState::CONSTRUCTING) {
        try {
            // unprepare processors
            for (auto &it : this->engines_) {
                it.second.second->processor()->Unprepare(global_context_);
                LOG(DEBUG) << "Successfully unprepared processor " << it.first;
            }
        } catch (...) {
            connections_.clear();
            engines_.clear();
            set_state(GraphState::NOGRAPH);
            throw InvalidGraphError("Error while unpreparing processors. Forced destruction of graph. Possible corruption of internal state.");
        }
    }
    
    // destroy connections and processors
    connections_.clear();
    engines_.clear();
    
    yaml_ = YAML::Null;
    
    LOG(INFO) << "Graph has been destroyed.";
    
    set_state(GraphState::NOGRAPH);
}

void ProcessorGraph::StartProcessing( std::string run_group_id, std::string run_id, std::string template_id, bool test_flag ) {
    
    // start processing only if state is READY
    
    if (state_ == GraphState::READY ) {
        
        // construct RunInfo object
        //runinfo_.reset( new RunInfo( terminate_signal_, context_, run_identifier, destination, source ) );
        run_context_.reset( new RunContext( global_context_, terminate_signal_, run_group_id, run_id, template_id, test_flag ) );
        
        set_state(GraphState::STARTING);
        
        // prepare all processors for processing
        // (i.e. flush buffers)
        for ( auto& it : this->engines_ ) {
            it.second.second->PrepareProcessing();
            LOG(DEBUG) << "Prepared data stream ports of processor " << it.first;    
        }
        LOG(INFO) << "Prepared all data stream ports for processing.";
        
        try {
            //loop through all processors
            for ( auto& it : this->engines_ ) {
                it.second.second->Start(*run_context_);
                LOG(DEBUG) << "Started thread for processor " << it.first;
            }
            LOG(INFO) << "Started all processors.";
        } catch( ... ) {
            StopProcessing();
            throw;
        }
        
        //wait until all processors are in running state
        while (!all_processors_running()) {
            if (run_context_->terminated()) {
                // processor terminated during preparation or preprocessing
                // other processors need to be unlocked still
                break;
            }
        }
        
        // all processors have either passed the preprocessing step
        // or have terminated with error, which will be dealt with in graphmanager::run
        // let's signal everyone to GO
        {
            std::unique_lock<std::mutex> lock(run_context_->mutex);
            run_context_->go_signal = true;
            run_context_->go_condition.notify_all();
        }
        
        set_state(GraphState::PROCESSING);
        
    } else if (state_ == GraphState::NOGRAPH || state_ == GraphState::CONSTRUCTING || state_ == GraphState::PREPARING) {
        throw InvalidStateError("Graph is not yet assembled.");
    } else { // STARTING, STOPPING
        // pass
    }
}

void ProcessorGraph::StopProcessing( ) {
   
    if (state_ == GraphState::PROCESSING || state_ == GraphState::STARTING)
    {
        set_state(GraphState::STOPPING);
        
        if (run_context_->error()) {
            LOG(ERROR) << "Processing terminated with error. " << run_context_->error_message();
        }
        
        // signal stop
        run_context_->Terminate();
        
        // alert waiting processors
        for ( auto& it : this->engines_ ) {
            it.second.second->Alert();
        }
        // join processor threads
        for ( auto& it : this->engines_ ) {
            it.second.second->Stop();
        }
        
        LOG(INFO) << "Stopped all processors.";
        LOG(INFO) << "Graph was processing for " << std::to_string( run_context_->seconds() ) << " seconds";
        
        run_context_.reset();
        terminate_signal_.store(false);
        
        set_state(GraphState::READY);
        
    } else if (state_ == GraphState::NOGRAPH || state_ == GraphState::CONSTRUCTING || state_ == GraphState::PREPARING) {
        throw InvalidStateError("Graph is not yet assembled.");
    } else { // READY, STOPPING 
        // pass
    }
    
}

void ProcessorGraph::Update( YAML::Node& node ) {
    
    // YAML
    // processor:
    //     state: value
    
        // make sure node is a map
    if (!node.IsMap()) {
        throw InvalidProcessorError("No processors found in state definition.");
    }
    // loop through all processors, make sure value is another map
    for(YAML::iterator it=node.begin();it!=node.end();++it) {
        std::string processor_name = it->first.as<std::string>();
        
        if (!it->second.IsMap()) { 
            LOG(ERROR) << "Invalid method definition for processor " << processor_name;
            continue;
        }
        // find corresponding processor engine
        if (engines_.count( processor_name )==0) {
            LOG(ERROR) << "In method definition: no processor named " << processor_name;
            continue;
        }
        
        ProcessorEngine* engine = engines_[processor_name].second.get();
        
        // loop through all states
        for (YAML::iterator it2=it->second.begin();it2!=it->second.end();++it2) {
            try {
                it2->second = engine->UpdateState( it2->first.as<std::string>(), it2->second.as<std::string>() );
            } catch ( std::exception & e ) {
                it2->second = false;
                LOG(ERROR) << "Unable to update state value: " << e.what();
            }
        }
    }
}

void ProcessorGraph::Retrieve( YAML::Node& node ) {
    
    // YAML
    // processor:
    //    state: <null>
    //return;
    // make sure node is a map
    if (!node.IsMap()) {
        throw InvalidProcessorError("No processors found in state definition.");
    }
    // loop through all processors, make sure value is another map
    for(YAML::iterator it=node.begin();it!=node.end();++it) {
        std::string processor_name = it->first.as<std::string>();
        
        if (!it->second.IsMap()) { 
            LOG(ERROR) << "Invalid method definition for processor " << processor_name;
            continue;
        }
        // find corresponding processor engine
        if (engines_.count( processor_name )==0) {
            LOG(ERROR) << "In method definition: no processor named " << processor_name;
            continue;
        }
        
        ProcessorEngine* engine = engines_[processor_name].second.get();
        
        // loop through all states
        for (YAML::iterator it2=it->second.begin();it2!=it->second.end();++it2) {
            try {
                it2->second = engine->RetrieveState( it2->first.as<std::string>() );
            } catch ( std::exception & e ) {
                it2->second = YAML::Null;
                LOG(ERROR) << "Unable to retrieve state value: " << e.what();
            }
        }
    }
}

void ProcessorGraph::Apply( YAML::Node& node ) {
   
    // YAML
    // processor:
    //     method:
    //         parameter: value
    
    if (!node.IsMap()) {
        throw InvalidProcessorError("No processors found in method definition.");
    }
    // loop through all processors, make sure value is another map
    for(YAML::iterator it=node.begin();it!=node.end();++it) {
        std::string processor_name = it->first.as<std::string>();
        
        if (!it->second.IsMap()) { 
            LOG(ERROR) << "Invalid method definition for processor " << processor_name;
            continue;
        }
        // find corresponding processor engine
        if (engines_.count( processor_name )==0) {
            LOG(ERROR) << "In method definition: no processor named " << processor_name;
            continue;
        }
        
        ProcessorEngine* engine = engines_[processor_name].second.get();
        
        // loop through all states
        for (YAML::iterator it2=it->second.begin();it2!=it->second.end();++it2) {
            try {
                it2->second = engine->ApplyMethod( it2->first.as<std::string>(), it2->second );
            } catch ( std::exception & e ) {
                it2->second = YAML::Null;
                LOG(ERROR) << "Unable to apply method: " << e.what();
            }
        }
    }
}

std::string ProcessorGraph::ExportYAML() {
    
    std::string s="";
    YAML::Node node;
    YAML::Emitter out;
    
    if (state_!=GraphState::NOGRAPH) {
    
        for ( auto& it : this->engines_ ) {
            node["processors"][it.first] = it.second.second->ExportYAML( );
            node["processors"][it.first]["class"] = it.second.first;
            
            if (yaml_["processors"][it.first]["options"]) {
                node["processors"][it.first]["options"] = yaml_["processors"][it.first]["options"];
            }
            
            if (yaml_["processors"][it.first]["advanced"]) {
                node["processors"][it.first]["advanced"] = yaml_["processors"][it.first]["advanced"];
            }
            
        }
        
        for (auto& it : this->connections_ ) {
            node["connections"].push_back( it->string() );
        }
        
        out << node;
        s = out.c_str();
    }
    
    return s;
    
}


