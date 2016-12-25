#include "configuration.hpp"

#include <regex>
#include <stdlib.h>
#include <sys/stat.h>
#include <fstream>

void FalconConfiguration::to_yaml( YAML::Node & node ) const {
    
    node["debug"]["enabled"] = debug_enabled;
    
    node["testing"]["enabled"] = testing_enabled;
    
    node["graph"]["autostart"] = graph_autostart;
    node["graph"]["file"] = graph_file;
    
    node["network"]["port"] = network_port;
    
    node["logging"]["path"] = logging_path;
    node["logging"]["screen"]["enabled"] = logging_screen_enabled;
    node["logging"]["cloud"]["enabled"] = logging_cloud_enabled;
    node["logging"]["cloud"]["port"] = logging_cloud_port;
    
    node["server_side_storage"]["environment"] = server_side_storage_environment;
    node["server_side_storage"]["resources"] = server_side_storage_resources;
}
    
void FalconConfiguration::from_yaml( const YAML::Node & node ) {
    
    debug_enabled = configuration::validate_bool_option( node["debug"]["enabled"], "debug:enabled", debug_enabled );
    testing_enabled = configuration::validate_bool_option( node["testing"]["enabled"], "testing:enabled", testing_enabled );
    graph_autostart = configuration::validate_bool_option( node["graph"]["autostart"], "graph:autostart", graph_autostart );
    graph_file = node["graph"]["file"].as<std::string>( graph_file );
    
    network_port = configuration::validate_number_option<int>( node["network"]["port"], "network:port", 1, 65535, network_port );
    
    logging_path = configuration::validate_path_option( node["logging"]["path"], "logging:path", false, logging_path );
    logging_screen_enabled = configuration::validate_bool_option( node["logging"]["screen"]["enabled"], "logging:screen:enabled", logging_screen_enabled );
    logging_cloud_enabled = configuration::validate_bool_option( node["logging"]["cloud"]["enabled"], "logging:cloud:enabled", logging_cloud_enabled );
    logging_cloud_port = configuration::validate_number_option<int>( node["logging"]["cloud"]["port"], "logging:cloud:port", 1, 65535, logging_cloud_port );
    
    server_side_storage_environment = configuration::validate_path_option( node["server_side_storage"]["environment"], "server_side_storage:environment", false, server_side_storage_environment );
    server_side_storage_resources = configuration::validate_path_option( node["server_side_storage"]["resources"], "server_side_storage:resources", true, server_side_storage_resources );
    
    if (node["server_side_storage"]["custom"]) {
        try {
            server_side_storage_custom = node["server_side_storage"]["custom"].as<std::map<std::string,std::string>>( );
        } catch ( YAML::BadConversion & e ) {
            throw configuration::ValidationError( "Invalid map of custom server side storage URIs in configuration." );
        }
    }
}

