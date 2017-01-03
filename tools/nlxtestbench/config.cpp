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

#include "config.hpp"

#include "filesource.hpp"
#include "whitenoisesource.hpp"
#include "squaresource.hpp"
#include "sinesource.hpp"

TestBenchConfiguration::TestBenchConfiguration() {}

void TestBenchConfiguration::to_yaml( YAML::Node & node ) const {
    
    node["network"]["ip"] = ip_address;
    node["network"]["port"] = port;
    
    node["stream"]["rate"] = stream_rate;
    node["stream"]["npackets"] = npackets;
    node["stream"]["autostart"] = autostart;
    
    //node["sources"]
    for (auto & it : sources) {
        node["sources"].push_back( it->to_yaml() );
    }
}

void TestBenchConfiguration::from_yaml( const YAML::Node & node ) {
    
    ip_address = node["network"]["ip"].as<std::string>( ip_address );
    port = configuration::validate_number_option<int>( node["network"]["port"], "network:port", 1, 65535, port );
    
    stream_rate = configuration::validate_number_option<double>( node["stream"]["rate"], "stream:rate", 0.01, 100000, stream_rate );
    npackets = node["stream"]["npackets"].as<uint64_t>( npackets );
    autostart = node["stream"]["autostart"].as<std::string>( autostart );
    
    std::string source_name;
    std::string source_class;
    
    sources.clear();
    
    //node["sources"]
    if (!node["sources"]) {
        // create default data sources
        sources.push_back( std::unique_ptr<DataSource>( new WhiteNoiseSource( 0.0, 1.0, 32000.0 ) ) );
    } else if (!node["sources"].IsSequence()) {
        throw configuration::ValidationError("Could not read sources.");
    } else {
        for(YAML::const_iterator it=node["sources"].begin();it!=node["sources"].end();++it) {
            //source_name = it->first.as<std::string>();
            if (!it->IsMap() || !(*it)["class"] ) {
                throw configuration::ValidationError("Please specify class of source");
            } else {
                source_class = (*it)["class"].as<std::string>();
                if (source_class=="nlx") {
                    sources.push_back( std::unique_ptr<DataSource>( FileSource::from_yaml((*it)["options"]) ) );
                } else if (source_class=="noise") {
                    sources.push_back( std::unique_ptr<DataSource>( WhiteNoiseSource::from_yaml((*it)["options"]) ) );
                } else if (source_class=="sine") {
                    sources.push_back( std::unique_ptr<DataSource>( SineSource::from_yaml((*it)["options"]) ) );
                } else if (source_class=="square") {
                    sources.push_back( std::unique_ptr<DataSource>( SquareSource::from_yaml((*it)["options"]) ) );
                }
                
            }
        }
    }
}

