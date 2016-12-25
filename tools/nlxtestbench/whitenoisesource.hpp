#ifndef WHITENOISESOURCE_H
#define WHITENOISESOURCE_H

#include <random>

#include "common.hpp"
#include "datasource.hpp"

class WhiteNoiseSource : public DataSource {
public:
    WhiteNoiseSource( double mean = 0.0, double stdev = 1.0, double sampling_rate = 1.0 );
    
    virtual std::string string();
    
    virtual bool Produce( char** data );
    
    virtual YAML::Node to_yaml() const;
    
    static WhiteNoiseSource* from_yaml( const YAML::Node node );
    
protected:
    double mean_;
    double stdev_;
    double sampling_rate_;
    uint64_t timestamp_ = 0;
    uint64_t delta_;
    
    NlxSignalRecord record_;
    
    char buffer_[BUFFERSIZE];
    
    std::default_random_engine generator_;
    std::normal_distribution<double> distribution_;
};

#endif // WHITENOISESOURCE_H
