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
