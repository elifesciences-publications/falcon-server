#ifndef SQUARESOURCE_H
#define SQUARESOURCE_H

#include <random>

#include "common.hpp"
#include "datasource.hpp"

class SquareSource : public DataSource {
public:
    SquareSource( double offset = 0.0, double amplitude = 1.0, double frequency = 1.0, double duty_cycle = 0.5, double sampling_rate = 1.0, double noise_stdev = 0 );
    
    virtual std::string string();
    
    virtual bool Produce( char** data );
    
    virtual YAML::Node to_yaml() const;
    
    static SquareSource* from_yaml( const YAML::Node node );
    
protected:
    double offset_;
    double amplitude_;
    double frequency_;
    double duty_cycle_;
    double sampling_rate_;
    double noise_stdev_;
    
    uint64_t timestamp_ = 0;
    uint64_t delta_;
    
    uint64_t counter_;
    double current_amplitude_;
    
    NlxSignalRecord record_;
    
    char buffer_[BUFFERSIZE];
    
    std::default_random_engine generator_;
    std::normal_distribution<double> distribution_;
};

#endif // SQUARESOURCE_H
