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

#ifndef DATASOURCE_H
#define DATASOURCE_H

#include "yaml-cpp/yaml.h"

#include <string>

class DataSource {
public:
    virtual ~DataSource() {}
    
    virtual bool Produce( char** data ) = 0;
    
    virtual std::string string() = 0;
    
    virtual YAML::Node to_yaml() const = 0;

};

#endif // DATASOURCE_H
