#include <unistd.h>
#include <deque>

#include "commandhandler.hpp"

#include "g3log/src/g2log.hpp"
#include "utilities/zmqutil.hpp"
#include "yaml-cpp/yaml.h"

using namespace commands;

bool CommandHandler::DelegateGraphCommand( std::deque<std::string>& command, std::deque<std::string>& reply ) {
    
    // delegate
    s_send_multi( *graph_socket_, command );
    // get reply
    reply = s_blocking_recv_multi( *graph_socket_ );
    
    return false;
}

bool CommandHandler::HandleCommand( std::deque<std::string>& command, std::deque<std::string>& reply ) {
    
    bool finished = false;
    
    std::deque<std::string> local_command;
    std::deque<std::string> local_reply;
    
    if (command.empty()) { return finished; }
    
    if ( command[0] == "graph" ) {
        // delegate
        command.pop_front();
        finished = DelegateGraphCommand( command, reply );
    } else if ( command[0]=="test" ) {
        if (command.size()>1) {
            if (command[1]=="true" || command[1]=="on") {
                global_context_->set_test(true);
                reply.push_back("OK");
            } else if (command[1]=="false" || command[1]=="off") {
                global_context_->set_test(false);
                reply.push_back("OK");
            } else {
                reply.push_back("ERR");
                reply.push_back("Invalid argument for test command");
            }
        } else {
            // toggle test flag
            global_context_->set_test( !global_context_->test() );
            reply.push_back("OK");
        }
    } else if ( command[0]=="quit" || command[0]=="kill" ) {
        local_command.push_back( "state" );
        DelegateGraphCommand( local_command, local_reply );
        
        if ( local_reply[0] == "PROCESSING" && command[0]=="quit" ) {
            // error
            reply.push_back("ERR");
            reply.push_back("Cannot quit while graph is processing.");
        } else {
            if (local_reply[0] == "PROCESSING") {
                local_command.back()="stop";
                DelegateGraphCommand( local_command, local_reply );
            }
            
            local_command.back() = "destroy";
            DelegateGraphCommand( local_command, local_reply );
            
            reply.push_back("OK");
            finished = true;
        }
    } else if ( command[0]=="info") {
        local_command.push_back( "state" );
        DelegateGraphCommand( local_command, local_reply );
        
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "run_environment_root" << YAML::Value << global_context_->storage_context("runroot");
        out << YAML::Key << "resource_root" << YAML::Value << global_context_->storage_context("resources");
        out << YAML::Key << "graph_state" << YAML::Value << local_reply[0];
        out << YAML::Key << "default_test_flag" << YAML::Value << global_context_->test();
        
        reply.push_back( std::string( out.c_str() ) );
    } else {
        // error
        reply.push_back("ERR");
        reply.push_back("Unknown command \"" + command[0] + "\".");
    }
    
    return finished;
    
}

void CommandHandler::start() {
    
    if (sources_.size() == 0) { return; }
    
    // connect to graph thread
    // construct socket here, so that it is automatically destructed when this function ends
    zmq::socket_t socket( global_context_->zmq(), ZMQ_REQ );
    socket.connect( "inproc://graph" );
    
    // save pointer to socket, so that DelegateGraphCommand can use it
    graph_socket_ = &socket;
    
    bool finished = false;
    std::deque<std::string> command;
    std::deque<std::string> reply;
    
    while(!finished) {
        
        usleep(100000); // 0.1 second
        
        // iterate through sources
        for (auto& it : sources_) {
            // retrieve command
            command.clear();
            
            if ( it->getcommand( command ) ) {
                reply.clear();
                finished = HandleCommand( command, reply );
                it->sendreply( command, reply );
            }
            
            if (finished) {break;}
            
        }
    }
    
    graph_socket_ = nullptr;
    
}
