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

#include "datasource.hpp"

#include <memory>
#include <vector>

class TestBenchConfiguration : public configuration::Configuration {

public:
    TestBenchConfiguration();

public:
    
    std::string ip_address = "127.0.0.1";
    int port = 5000;
    double stream_rate = 32000.0;
    uint64_t npackets = 0;
    std::string autostart = "";
    
    std::vector<std::unique_ptr<DataSource>> sources;
    
public:
    virtual void from_yaml( const YAML::Node & node ) override;
    virtual void to_yaml( YAML::Node & node ) const override;
};
