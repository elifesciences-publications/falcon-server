#ifndef PROCESSORENGINE_H
#define PROCESSORENGINE_H

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <map>
#include <utility>

#include "threadutilities.hpp"

#include "runinfo.hpp"
//#include "iprocessor.hpp"
#include "yaml-cpp/yaml.h"

// forward declaration
class IProcessor;

class ProcessorEngine final {
public:
    ProcessorEngine( std::string name, std::unique_ptr<IProcessor> processor );
    ~ProcessorEngine();
    
    const std::string name() const { return name_; }
    
    void ThreadEntry(RunContext& runcontext);
    
    void Configure(const YAML::Node& node, const GlobalContext& context);
    void CreatePorts();
    
    void NegotiateConnections();
    void CreateRingBuffers();
    
    void PrepareProcessing();
    
    void Start(RunContext& runcontext);
    void Stop();
    
    void Alert();
    
    YAML::Node ExportYAML();
    
    bool UpdateState( std::string name, std::string value );
    std::string RetrieveState( std::string name );
    YAML::Node ApplyMethod( std::string name, const YAML::Node& node );
    
    IProcessor* processor() { return processor_.get(); }
    
    bool running() const { return running_.load(); };
    
protected:
    std::atomic<bool> running_;
    
    bool negotiated_ = false;
    bool prepared_ = false;
        
    std::thread thread_;
    std::string name_;
    
    std::unique_ptr<IProcessor> processor_;
    
    std::atomic<bool> has_test_flag_;
    std::atomic<bool> test_flag_;
    
    ThreadPriority thread_priority_;
    ThreadCore thread_core_;
    
    std::map<std::string, int> requested_buffer_sizes_;
    
};

typedef std::map<std::string, std::pair< std::string, std::unique_ptr<ProcessorEngine>>> ProcessorEngineMap;

#endif
