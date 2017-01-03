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

#ifndef ZMQUTIL_H
#define ZMQUTIL_H

#include <zmq.hpp>
#include <cstring>
#include <deque>

typedef std::deque<std::string> zmq_frames;

// Receive 0MQ string from socket and convert into string
std::string s_recv (zmq::socket_t & socket);

// Non-blocking receive string
bool s_nonblocking_recv( zmq::socket_t & socket, std::string & s_message );

// Convert string to 0MQ string and send to socket
bool s_send (zmq::socket_t & socket, const std::string & string);

// Sends string as 0MQ string, as multipart non-terminal
bool s_sendmore (zmq::socket_t & socket, const std::string & string);

// Send multi-part message
bool s_send_multi( zmq::socket_t & socket, const zmq_frames& frames);

// helper function to check if more message parts are available
bool sockopt_rcvmore( zmq::socket_t & socket);

// Receive multipart message
zmq_frames s_blocking_recv_multi( zmq::socket_t & socket);

// Non-blocking receive multi-part message
bool s_nonblocking_recv_multi( zmq::socket_t & socket, zmq_frames &frames);

#endif // ZMQUTIL_H
