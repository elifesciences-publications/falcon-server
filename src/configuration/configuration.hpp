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

