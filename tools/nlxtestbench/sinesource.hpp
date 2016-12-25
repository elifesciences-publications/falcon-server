#ifndef SINESOURCE_H
#define SINESOURCE_H

#include <random>

#include "common.hpp"
#include "datasource.hpp"

class SineSource : public DataSource {
public:
    SineSource( double offset = 0.0, double amplitude = 1.0, double frequency = 1.0, double sampling_rate = 1.0, double noise_stdev = 0 );
    
    virtual std::string string();
    
    virtual bool Produce( char** data );
    
    virtual YAML::Node to_yaml() const;
    
    static SineSource* from_yaml( const YAML::Node node );
    
protected:
    double offset_;
    double amplitude_;
    double frequency_;
    double sampling_rate_;
    double noise_stdev_;
    
    uint64_t timestamp_ = 0;
    uint64_t delta_;
    
    NlxSignalRecord record_;
    
    double omega_;
    
    char buffer_[BUFFERSIZE];
    
    std::default_random_engine generator_;
    std::normal_distribution<double> distribution_;
};

#endif // SQUARESOURCE_H
