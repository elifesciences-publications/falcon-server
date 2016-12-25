#ifndef COMMANDSOURCE_H
#define COMMANDSOURCE_H

#include <deque>
#include <cstring>

#include "utilities/keyboard.hpp"
#include "cppzmq/zmq.hpp"

namespace commands
{

class CommandSource {
public:
    virtual bool getcommand( std::deque<std::string> & command ) {return false;}
    virtual bool sendreply( const std::deque<std::string> & command, std::deque<std::string> &reply) {return false;}
};

class CommandLineCommands: public CommandSource {
public:
    
    void AddCommand( std::deque<std::string> command );
    
    bool getcommand( std::deque<std::string> & command );
    bool sendreply( const std::deque<std::string> & command, std::deque<std::string> & reply );

protected:
    std::deque<std::deque<std::string>> queued_commands_;
};

class KeyboardCommands: public CommandSource {
public:

    KeyboardCommands();
    ~KeyboardCommands();

    bool getcommand( std::deque<std::string> & command );
    bool sendreply( const std::deque<std::string> & command, std::deque<std::string> & reply );
};

class ZMQCommands: public CommandSource {
public:
    zmq::socket_t *socket;
    
    ZMQCommands (zmq::context_t &context, int port);
    ~ZMQCommands();
    
    bool getcommand( std::deque<std::string> & command );
    bool sendreply( const std::deque<std::string> & command, std::deque<std::string> & reply );
};

} //namespace commands

#endif // COMMANDSOURCE_H
