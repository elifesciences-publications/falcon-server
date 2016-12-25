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
