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

#include <unistd.h>
#include <fstream>

#include "graphmanager.hpp"
#include "utilities/zmqutil.hpp"
#include "utilities/general.hpp"

#include "g3log/src/g2log.hpp"

using namespace graph;

GraphManager::GraphManager( GlobalContext& context ) : global_context_(&context), graph_(context) {
    
    // register all processors
    registerProcessors();
}

void GraphManager::HandleCommand( std::string command, std::deque<std::string>& extra, std::deque<std::string>& reply ) {
    
    if (command == "build") {
        if (extra.size()<1) { throw std::runtime_error( "Missing YAML graph definition." ); }
        
        auto node = YAML::Load( extra[0] );        
        graph_.Build( node );
        
        // save YAML to global_context_.resolve_path( "graphs://_last_graph" )
        std::ofstream fout( global_context_->resolve_path( "graphs://_last_graph" ) );
        fout << extra[0];
            
    } else if (command == "buildfile") {
        if (extra.size()<1) { throw std::runtime_error( "Missing YAML graph definition file." ); }

        std::string file = global_context_->resolve_path(extra[0], "graphs");
        
        try {
            auto node = YAML::LoadFile( file );
            
            graph_.Build( node );
            
            // save YAML to global_context_.resolve_path( "graphs://_last_graph" )
            // copy file
            std::ifstream source(file, std::ios::binary);
            std::ofstream dest(global_context_->resolve_path( "graphs://_last_graph" ), std::ios::binary);
            dest << source.rdbuf();
            
            
        } catch (YAML::BadFile& e) {
            throw std::runtime_error( "Cannot open YAML graph definition file "
                + file + ". Check if file actually exists.");
        }
        
        

    } else if (command == "destroy") {
        graph_.Destroy();
    } else if (command == "start" || command == "test") {
        std::string run_env = extra.size()>0 ? extra[0] : "";
        std::string destination = extra.size()>1 ? extra[1] : "";
        std::string source = extra.size()>2 ? extra[2] : "";
        graph_.StartProcessing( run_env, destination, source, command=="test" || global_context_->test() );
    } else if (command == "stop") {
        graph_.StopProcessing();
    } else if (command == "state") {
        reply.push_back( graph_.state_string() );
    } else if (command == "update") {
        if (extra.size()>0) { 
            YAML::Node node = YAML::Load( extra[0] );
            graph_.Update( node );
            YAML::Emitter out;
            out << node;
            reply.push_back(std::string( out.c_str() ));
        }
    } else if (command == "retrieve") {
        if (extra.size()>0) {
            YAML::Node node = YAML::Load( extra[0] );
            graph_.Retrieve( node );
            YAML::Emitter out;
            out << node;
            reply.push_back(std::string( out.c_str() ));
        }
    } else if (command == "apply") {
        if (extra.size()>0) {
            YAML::Node node = YAML::Load( extra[0] );
            graph_.Apply( node );
            YAML::Emitter out;
            out << node;
            reply.push_back(std::string( out.c_str() ));
        }
    } else if (command == "yaml") {
        reply.push_back( graph_.ExportYAML() );
    } else {
        throw std::runtime_error( "Unknown graph command \"" + command + "\"." );
    }
    
}

void GraphManager::Run() {
    
    //initialize
    zmq::socket_t socket( global_context_->zmq(), ZMQ_REP);
    socket.bind ("inproc://graph");
    
    zmq_frames request;
    zmq_frames reply;
    
    while (!terminated()) {
        
        // sleep a bit, since we are continuously polling
        usleep(1000); // 1 msec
        
        //process commands
        request.clear();
        if ( s_nonblocking_recv_multi( socket, request) ) {
            // handle command
            
            reply.clear();
            
            std::string command = request[0];
            request.pop_front();
            
            LOG(DEBUG) << "GraphManager received command \"" << command << "\"";
            
            try {
                
                HandleCommand( command, request, reply );
                
                if (reply.size()==0) {
                    reply.push_back( "OK" );
                }
                
            } catch ( GraphException &e ) {
                if ( e.isFatal() ) {
                    reply.push_back( "ERR" );
                } else {
                    reply.push_back( "WARN" );
                }
                reply.push_back( e.gettype() );
                reply.push_back( e.what() );
                
                LOG(ERROR) << "Error handling command: " << command << " Error type: " << e.gettype() << "  Error: " << e.what();
                
            } catch ( std::exception &e ) {
                reply.push_back("ERR");
                reply.push_back("exception");
                reply.push_back(e.what());
                
                LOG(ERROR) << "Error handling command: " << command << " Error: " << e.what();
                
            } catch ( ... ) {
                reply.push_back( "ERR" );
                reply.push_back( "Unknown" );
                reply.push_back( "Unknown error." );
                
                LOG(ERROR) << "Error handling command: " << command;
            }
             
            // reply
            s_send_multi( socket, reply );

            LOG(DEBUG) << "GraphManager replied to command \"" << command << "\" with \"" << join( reply.begin(), reply.end(), std::string(" | ") ) << "\"";

        }
        
        //check if graph processing was terminated by a processor
        //or if all processors are done and waiting to be killed
        if ( graph_.done() ) {
            LOG(DEBUG) << "Processing is done.";
            graph_.StopProcessing();
        }
    }
    
    //finish
    
}
