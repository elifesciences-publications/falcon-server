#include "utilities/configuration.hpp"
#include <map>

class FalconConfiguration : public configuration::Configuration {

public:
    
    std::string graph_file = "";
    bool debug_enabled = false;
    bool testing_enabled = false;
    bool graph_autostart = false;
    int network_port = 5555;
    std::string logging_path = "./";
    bool logging_screen_enabled = true;
    bool logging_cloud_enabled = true;
    int logging_cloud_port = 5556;
    std::string server_side_storage_environment = "./";
    std::string server_side_storage_resources = "$HOME/.falcon";
    std::map<std::string,std::string> server_side_storage_custom;
    
public:
    virtual void from_yaml( const YAML::Node & node ) override;
    virtual void to_yaml( YAML::Node & node ) const override;
};

