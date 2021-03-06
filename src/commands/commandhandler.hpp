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
