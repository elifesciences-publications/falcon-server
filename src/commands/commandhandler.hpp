#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include "commandsource.hpp"
#include "../context.hpp"

namespace commands {

class CommandHandler {
public:
    CommandHandler( GlobalContext& context ) { 
        global_context_ = &context;
    }

    void addSource( CommandSource& source ) { sources_.push_back( &source ); }

    bool HandleCommand( std::deque<std::string>& command, std::deque<std::string>& reply );
    bool DelegateGraphCommand( std::deque<std::string>& command, std::deque<std::string>& reply );
    
    void start();
    
private:
    typedef std::vector<CommandSource*> VectSources;
    VectSources sources_;
    GlobalContext *global_context_;
    zmq::socket_t * graph_socket_;
};

} //namespace commands


#endif //COMMANDHANDLER_H
