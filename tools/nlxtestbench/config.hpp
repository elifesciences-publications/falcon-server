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
