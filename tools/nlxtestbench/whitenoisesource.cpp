
#include "whitenoisesource.hpp"
#include "utilities/string.hpp"

WhiteNoiseSource::WhiteNoiseSource( double mean, double stdev, double sampling_rate ) : 
    mean_(mean), stdev_(stdev), sampling_rate_(sampling_rate), delta_(1000000/sampling_rate), distribution_(mean, stdev) {}
    
std::string WhiteNoiseSource::string() {
    
    return "gaussian white noise (fs = " + to_string_n(sampling_rate_) + " Hz, " +
           "mean = " + to_string_n(mean_) + " uV, " +
           "stdev = " + to_string_n(stdev_) + "uV)";
}
    
bool WhiteNoiseSource::Produce( char** data ) {
    
    record_.set_data( distribution_(generator_) );
    record_.set_timestamp( timestamp_ );
    timestamp_ = timestamp_ + delta_;
    record_.ToNetworkBuffer( buffer_, BUFFERSIZE );
    *data = buffer_;
    return true;
}

YAML::Node WhiteNoiseSource::to_yaml() const {
    
    YAML::Node node;
    
    node["mean"] = mean_;
    node["stdev"] = stdev_;
    node["sampling_rate"] = sampling_rate_;
    
    return node;
}

WhiteNoiseSource* WhiteNoiseSource::from_yaml( const YAML::Node node ) {
    
    return new WhiteNoiseSource(    node["mean"].as<double>(0.0),
                                    node["stdev"].as<double>(1.0),
                                    node["sampling_rate"].as<double>(32000) );
}