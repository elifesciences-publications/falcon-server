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

#ifndef FILESOURCE_H
#define FILESOURCE_H

#include <string>
#include <fstream>

#include "common.hpp"
#include "datasource.hpp"

class FileSource : public DataSource {
    
public:

    FileSource( std::string file, bool cycle );
    
    ~FileSource();
    
    virtual std::string string();
    
    std::string file() const;
    
    virtual bool Produce( char** data );
    
    virtual YAML::Node to_yaml() const;
    
    static FileSource* from_yaml( const YAML::Node node );
    
protected:
    std::string file_;
    bool cycle_;
    
    std::ifstream raw_data_file;
    
    char buffer_[NLX_PACKETBYTESIZE(128)];
};

#endif // FILESOURCE_H
