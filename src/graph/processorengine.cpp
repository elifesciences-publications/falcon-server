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

#include "processorengine.hpp"
#include "iprocessor.hpp"

#include "g3log/src/g2log.hpp"

ProcessorEngine::ProcessorEngine( std::string name, std::unique_ptr<IProcessor> processor ) :
    running_(false), thread_(), name_(name), processor_(std::move(processor)), 
    has_test_flag_(false), test_flag_(false),
    thread_priority_(PRIORITY_NONE), thread_core_(CORE_NOT_PINNED) {

    processor_->set_name(name_);
}

ProcessorEngine::~ProcessorEngine() {
    
    //TODO: make sure no exception is thrown
    
    // if running, stop
    Stop();
    
    // delete processor
    processor_.reset();
}

void ProcessorEngine::ThreadEntry( RunContext& runcontext ) {
    
    LOG(DEBUG) << "Entering thread for processor " << name_;
    
    ProcessingContext context( runcontext, name_, has_test_flag_.load() ? test_flag_.load() : runcontext.test() );
    
    LOG(DEBUG) << name_ << ": processor test flag set to " << context.test();
    
    PrepareProcessing();
    
    try {
        processor_->TestPrepare( context );
    } catch (std::exception& e) {
        context.TerminateWithError( "TestPrepare", e.what() );
    }
    
    try {
        processor_->Preprocess(context);
    } catch (std::exception& e) {
        context.TerminateWithError( "PreProcess", e.what() );
    }
    
    running_.store(true);

    // wait for the go signal
    {
        std::unique_lock<std::mutex> lock(runcontext.mutex);
        while (!runcontext.go_signal) { runcontext.go_condition.wait(lock); }
    }
    
    try {
        processor_->Process(context);
    } catch (std::exception& e) {
        context.TerminateWithError( "Process", e.what() );
    }
    
    try {
        processor_->Postprocess(context);
    } catch (std::exception& e) {
        context.TerminateWithError( "PostProcess", e.what() );
    }
    
    try {
        processor_->TestFinalize(context);
    } catch (std::exception& e) {
        //LOG(ERROR) << name_ << " (TestFinalize): " << e.what();
        context.TerminateWithError( "TestFinalize", e.what() );
    }
    
    running_.store(false);
    
    LOG(DEBUG) << "Exiting thread for processor " << name_;
}

void ProcessorEngine::Configure( const YAML::Node& node, const GlobalContext& context ) {
    
    if ( node["options"] ) {
        
        if (node["options"]["test"]) {
            has_test_flag_ = true;
            test_flag_ = node["options"]["test"].as<bool>();
        }
    }

    if (node["advanced"]) {
        thread_priority_ = node["advanced"]["threadpriority"].as<ThreadPriority>( processor_->default_thread_priority() );
        thread_core_ = node["advanced"]["threadcore"].as<ThreadCore>( CORE_NOT_PINNED );

        if (node["advanced"]["buffer_sizes"]) {
            requested_buffer_sizes_ = node["advanced"]["buffer_sizes"].as<std::map<std::string,int>>( );
        }
    } else {
        thread_priority_ = processor_->default_thread_priority();
        thread_core_ = CORE_NOT_PINNED;
    }
    
    processor_->Configure( node["options"], context );
}

void ProcessorEngine::CreatePorts() {
    
    processor_->CreatePortsInternal( requested_buffer_sizes_ );
}

void ProcessorEngine::NegotiateConnections() {
    
    processor_->NegotiateConnections();
}

void ProcessorEngine::CreateRingBuffers() {
    
    processor_->CreateRingBuffers();
}

void ProcessorEngine::PrepareProcessing() {
    
    processor_->PrepareProcessing();
}

bool ProcessorEngine::UpdateState( std::string name, std::string value ) {
    
    return processor_->UpdateState( name, value );
}

std::string ProcessorEngine::RetrieveState( std::string name ) {
    
    return processor_->RetrieveState( name );
}

YAML::Node ProcessorEngine::ApplyMethod( std::string name, const YAML::Node& node ) {
    
    return processor_->ApplyMethod( name, node );
}

void ProcessorEngine::Start(RunContext& runcontext) {

    if (!running_) {
        Stop();
        
        thread_ = std::thread( &ProcessorEngine::ThreadEntry, this, std::ref(runcontext) );
        
        if (!set_realtime_priority( thread_.native_handle(), thread_priority_)) {
            LOG(WARNING) << "Unable to set thread priority for " << name_;
        } else if (thread_priority_>=PRIORITY_MIN) {
            LOG(INFO) << "Successfully set thread priority for " << name_ << " to " << thread_priority_ << "%.";
        }
        
        if (!set_thread_core( thread_.native_handle(), thread_core_)) {
            LOG(WARNING) << "Unable to pin thread for " << name_ << " to core "
                << thread_core_;
        } else if (thread_core_>=0) {
            LOG(INFO) << "Successfully pinned thread for " << name_ << " to core "
                << thread_core_ << ".";
        }
        
        
    }
}

void ProcessorEngine::Stop() {
    
    if( thread_.joinable()) thread_.join();
    LOG(DEBUG) << name() << ": thread joined";
}

void ProcessorEngine::Alert() {
    
    processor_->Alert();
}

YAML::Node ProcessorEngine::ExportYAML( ) {
    
    return processor_->ExportYAML();
}
