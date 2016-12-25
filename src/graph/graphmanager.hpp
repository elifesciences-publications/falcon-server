#ifndef GRAPHMANAGER_H
#define GRAPHMANAGER_H

#include <thread>
#include <deque>

#include "../processors/registerprocessors.hpp"
#include "../context.hpp"

#include "processorgraph.hpp"

namespace graph
{

class GraphManager
{
public:
    
    GraphManager( GlobalContext& context );
    
    ~GraphManager() {  stop(); }
    
    void stop() { terminate_ = true; thread_.join(); }
    
    void start() { terminate_ = false; thread_ = std::thread( &GraphManager::Run, this ); }
    
    bool terminated() const { return terminate_; }
    
private:
    std::thread thread_;
    
    void Run();
    
    bool terminate_ = false;
    GlobalContext* global_context_;
    
    void HandleCommand( std::string command, std::deque<std::string>& extra, std::deque<std::string>& reply );
    
    ProcessorGraph graph_;
    
};
    
} // namespace graph

#endif
