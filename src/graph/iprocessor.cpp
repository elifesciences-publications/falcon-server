#include "iprocessor.hpp"
#include <regex>
#include <iostream>
#include <fstream>
#include "g3log/src/g2log.hpp"

#include "utilities/general.hpp"

bool is_valid_name( std::string s ) {
    
    return std::regex_match(s, std::regex("^\\w+$"));
}

const std::set<std::string> IProcessor::input_port_names() const {
    
    std::set<std::string> names;
    for (auto & it : input_ports_ ) {
        names.insert( it.first );
    }
    return names;
}

const std::set<std::string> IProcessor::output_port_names() const {
    
    std::set<std::string> names;
    for (auto & it : output_ports_ ) {
        names.insert( it.first );
    }
    return names;
}

std::string IProcessor::default_input_port() const {
    
    if (input_ports_.size()!=1) {
        throw std::runtime_error("Cannot determine default input port for processor \"" + name() + "\"." );
    }
    return input_ports_.begin()->first;
}

std::string IProcessor::default_output_port() const {
    
    if (output_ports_.size()!=1) {
        throw ProcessorInternalError("Cannot determine default output port.", name() );
    }
    return output_ports_.begin()->first;
}

void IProcessor::create_file( std::string prefix, std::string variable_name,
std::string extension ) {
    
    std::string full_path = prefix + "." + variable_name + "." + extension;
    if ( path_exists( full_path ) ) {
        throw ProcessorInternalError( "Output file " + full_path +
            " already exists.", name() );
    }

    // unique_ptr gives compilation error for unknown reasons
    auto stream = std::shared_ptr<std::ostream>(
        new std::ofstream( full_path, std::ofstream::out | std::ofstream::binary ) );
    if ( !stream->good() ) {
        throw ProcessorInternalError( "Error opening output file " +
            full_path + ".", name() );
    } else {
        LOG(INFO) << name() << ". " + full_path + " opened correctly for writing";
    }
    streams_[variable_name] = std::move(stream);
}

void IProcessor::prepare_latency_test( ProcessingContext& context ) {
    
    auto path = context.resolve_path( "test://", "test" );
    create_file( path + name(), "SourceTimestamps" );
    LOG(UPDATE) << name() << ". Resizing the source timestamp vector for testing ...";
    // reserve enough memory for the test
    test_source_timestamps_.resize(
        MAX_TEST_SAMPLING_FREQUENCY * (3600 * MAX_N_HOURS_TEST) );
    LOG(INFO) << name() << ". Source timestamp vector resized with " <<
        test_source_timestamps_.size() << " elements";
}

void IProcessor::save_source_timestamps_to_disk( std::uint64_t n_timestamps ) {
    
    test_source_timestamps_.resize( n_timestamps );
    for (auto source_ts: test_source_timestamps_) {
        streams_["SourceTimestamps"]->write(
            reinterpret_cast<const char*>( &source_ts ),
            sizeof(TimePoint) );
    }
    LOG(INFO) << name() << ". " << test_source_timestamps_.size() <<
        " source timestamps were written to disk.";
}

void IProcessor::NegotiateConnections() {
    
    if (!negotiated_) {
        // check if all input slots are connected
        for (auto & it : input_ports_ ) {
            for ( int k=0; k<it.second->number_of_slots(); ++k ) {
                if (!it.second->slot(k)->connected()) {
                    LOG(ERROR) << name() << ": input slot \"" << it.first + "." << std::to_string(k) << "\" is not connected.";
                    throw ProcessorInternalError( "input slot \"" + it.first + "." + std::to_string(k) + "\" is not connected.", name() );
                }
            }
        }
        
        for (auto & it : output_ports_ ) {
            for ( int k=0; k<it.second->number_of_slots(); ++k ) {
                if (!it.second->slot(k)->connected()) {
                    LOG(WARNING) << name() << ": output slot \"" << it.first + "." << std::to_string(k) << "\" is not connected.";
                }
            }
        }
        
        CompleteStreamInfo();
        
        for (auto & it : output_ports_ ) {
            for ( int k=0; k<it.second->number_of_slots(); ++k ) {
                if (!it.second->slot(k)->streaminfo().finalized()) {
                    LOG(ERROR) << name() << ": output slot \"" << it.first + "." << std::to_string(k) << "\" is not finalized.";
                    throw ProcessorInternalError( "output slot \"" + it.first + "." + std::to_string(k) + "\" is not finalized.", name() );
                }
            }
        }
        
        negotiated_ = true;
    }
}

void IProcessor::CompleteStreamInfo() {
    
    for (auto & it : output_ports_) {
        for ( int k=0; k<it.second->number_of_slots(); ++k ) {
            it.second->slot(k)->streaminfo().Finalize();
        }
    }
}

void IProcessor::CreateRingBuffers() {
    
    for (auto& it : output_ports_ ) {
        it.second->CreateRingBuffers();
    }
}

void IProcessor::PrepareProcessing() {
    
    for (auto& it : input_ports_ ) {
        it.second->PrepareProcessing();
    }
    
    // reset all output slot cursors to 0
    for (auto& it : output_ports_ ) {
        it.second->PrepareProcessing();
    }
}

void IProcessor::Alert() {
    
    for (auto& it : output_ports_ ) {
        it.second->UnlockSlots();
    }
    for (auto& it : input_ports_ ) {
        it.second->UnlockSlots();
    }
}


bool IProcessor::UpdateState( std::string state, std::string value ) {
    
    // look up state variable
    IState* pstate = this->shared_state(state);
    
    // check if externally settable??
    if (pstate->permissions().external()==Permission::WRITE) {
        // set from string
        return pstate->set_string( value );
    } else {
        throw ProcessorInternalError( "Shared state " + state + " can not be controlled externally.", name());
        //return false;
    }
}

std::string IProcessor::RetrieveState( std::string state ) {
    
    // look up state variable
    IState* pstate = this->shared_state(state);
    
    if (pstate->permissions().external()!=Permission::NONE) {
        return pstate->get_string();
    } else {
        throw ProcessorInternalError( "Shared state " + state + " can not be read externally.", name());
    }
}

YAML::Node IProcessor::ApplyMethod( std::string name, const YAML::Node& node ) {
    
    return exposed_method(name)(node);
}
    
YAML::Node IProcessor::ExportYAML() {
    
    YAML::Node node;
    
    for (auto & it : input_ports_ ) {
        node["inports"][it.first] = it.second->ExportYAML();
    }
    
    for (auto & it : output_ports_ ) {
        node["outports"][it.first] = it.second->ExportYAML();
    }
    
    for (auto & it : shared_states_ ) {
        if (it.second->permissions().external()==Permission::WRITE) {
            node["states"][it.first]["permission"] = "write";
        } else if (it.second->permissions().external()==Permission::READ) {
            node["states"][it.first]["permission"] = "read";
        }
        node["states"][it.first]["value"] = it.second->get_string();
        node["states"][it.first]["units"] = it.second->units();
        node["states"][it.first]["description"] = it.second->description();
    }
    
    for (auto & it : exposed_methods_ ) {
        node["methods"].push_back( it.first );
    }
    
    return node;
}

void IProcessor::CreatePortsInternal( std::map<std::string, int> & buffer_sizes ) {
    
    CreatePorts();
    // set requested buffer sizes
    for ( auto & it : buffer_sizes ) {
        if (!has_output_port( it.first ) || it.second<2) {
            LOG(WARNING) << "Could not set ringbuffer size to " << it.second << " for port " << name() << "." << it.first;
        } else {
            output_port( it.first )->set_buffer_size( it.second );
            LOG(INFO) << "Set ringbuffer size to " << it.second << " for port " << name() << "." << it.first;
        }
    }
}
