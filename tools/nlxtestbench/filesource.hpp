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
