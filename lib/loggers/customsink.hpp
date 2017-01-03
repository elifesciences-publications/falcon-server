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

#include <zmq.hpp>
#include <deque>

#include "g3log/src/g2log.hpp"
#include "g3log/src/g2logmessage.hpp" 

class ZMQSink {
public:
   ZMQSink(zmq::context_t& context, int port);
   virtual ~ZMQSink();
   
   std::deque<std::string> FormatMessage(g2::LogMessage &msg);
   void ReceiveLogMessage(g2::LogMessageMover message);

private:

   zmq::socket_t *publisher;

   ZMQSink& operator=(const ZMQSink&) = delete;
   ZMQSink(const ZMQSink& other) = delete;

};

class ScreenSink {
public:
   ScreenSink() {};
   virtual ~ScreenSink() {};
   
   std::string FormatMessage(g2::LogMessage &msg);
   void ReceiveLogMessage(g2::LogMessageMover message);

private:

   ScreenSink& operator=(const ScreenSink&) = delete;
   ScreenSink(const ScreenSink& other) = delete;

};
