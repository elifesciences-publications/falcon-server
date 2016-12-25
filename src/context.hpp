#ifndef CONTEXT_H
#define CONTEXT_H

#include <atomic>

#include <string>
#include <zmq.hpp>

#include "utilities/string.hpp"

class StorageContext {
public:
    StorageContext( std::string default_context = "" ) : default_context_(default_context) {}
    
    std::map<std::string, std::string> storage_map() const { return storage_map_; }
    
    std::string default_context() const { return default_context_; }
    void set_default_context( std::string c ) { default_context_ = c; }
    
    std::string storage_context( std::string c ) { return storage_map_[c]; }
    
    std::string resolve_path( const std::string& p ) const {
        return resolve_server_path( p, storage_map_, default_context_ );
    }
    std::string resolve_path( const std::string& p, std::string default_context ) const {
        return resolve_server_path( p, storage_map_, default_context );
    }

protected:
    void add_storage_context( std::string key, std::string value ) {
        storage_map_[key] = value;
    }
    
protected:
    std::string default_context_;
    std::map<std::string,std::string> storage_map_;
};

class GlobalContext : public StorageContext {
public:
    GlobalContext( bool test_flag, std::map<std::string,std::string> & uri ) : StorageContext(""), default_test_flag_(test_flag) { 
        
        zmq_context_ = zmq::context_t(1);
        for (auto & it : uri ) {
            add_storage_context( it.first, it.second );
        }
    }
    
    zmq::context_t& zmq() { return zmq_context_; }
    //std::string run_environment() const { return root_paths_["runenv"]; }
    //std::string resource_root() const { return root_paths["resources"]; }
    
    bool test() const { return default_test_flag_.load(); }
    void set_test( bool value ) { default_test_flag_ = value; }
    
private:
    zmq::context_t zmq_context_;
    std::atomic<bool> default_test_flag_;
};

#endif
