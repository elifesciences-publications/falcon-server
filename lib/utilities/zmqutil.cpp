
#include "zmqutil.hpp"

// Receive 0MQ string from socket and convert into string
std::string s_recv (zmq::socket_t & socket) {
    
    zmq::message_t message;
    socket.recv(&message);
    return std::string(static_cast<char*>(message.data()), message.size());
}

bool s_nonblocking_recv( zmq::socket_t & socket, std::string & s_message ) {
    
    zmq::message_t message;
    if ( socket.recv (&message, ZMQ_NOBLOCK) ) {
        s_message.assign( static_cast<char*>(message.data()), message.size());
        return true;
    }
    
    return false;
}

// Convert string to 0MQ string and send to socket
bool s_send (zmq::socket_t & socket, const std::string & string) {
    
    zmq::message_t message(string.size());
    memcpy (message.data(), string.data(), string.size());
    bool rc = socket.send (message);
    return (rc);
}

// Sends string as 0MQ string, as multipart non-terminal
bool s_sendmore (zmq::socket_t & socket, const std::string & string) {
    
    zmq::message_t message(string.size());
    memcpy (message.data(), string.data(), string.size());
    bool rc = socket.send (message, ZMQ_SNDMORE);
    return (rc);
}

bool s_send_multi( zmq::socket_t & socket, const zmq_frames& frames) {
    
    if(frames.size()==0)
        return true;
    
    // all frames but last one
    for( unsigned int i = 0; i < frames.size() - 1; ++i)
        if(!s_sendmore(socket, frames[i]))
            return false;
    // last frame
    return s_send(socket, frames.back());
}

bool sockopt_rcvmore( zmq::socket_t & socket) {
    
    int64_t rcvmore = 0;
    size_t type_size = sizeof(int64_t);
    socket.getsockopt(ZMQ_RCVMORE, &rcvmore, &type_size);
    return rcvmore ? true : false;
}

zmq_frames s_blocking_recv_multi( zmq::socket_t & socket) {
    
    zmq_frames frames;
    do {
        frames.push_back( s_recv( socket ) );        
    } while(sockopt_rcvmore(socket));
    return frames;
}

bool s_nonblocking_recv_multi( zmq::socket_t & socket, zmq_frames &frames) {
    
    zmq::message_t message;
    
    if ( !socket.recv (&message, ZMQ_NOBLOCK) ) { return false; }
    
    frames.push_back( std::string( static_cast<char*>(message.data()), message.size() ) );
    
    while(sockopt_rcvmore(socket)) {
        socket.recv (&message, ZMQ_NOBLOCK);
        frames.push_back( std::string( static_cast<char*>(message.data()), message.size() ));        
    } 
    
    return true;
}
