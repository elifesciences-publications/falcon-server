#include <stdexcept>
#include <string>
#include <limits>
#include <regex>
#include <iostream>

#include "yaml-cpp/yaml.h"

namespace configuration {

class ValidationError : public std::runtime_error {
public:
    ValidationError( std::string msg ) : runtime_error( msg ) {}
};


bool validate_bool_option( const YAML::Node & node, std::string option, bool default_value );

template <typename T>
T validate_number_option( const YAML::Node & node, std::string option, T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max(), T default_value = 0 );

std::string validate_path_option( std::string filename, std::string option, bool create = false );

std::string validate_path_option( const YAML::Node & node, std::string option, bool create = false, std::string default_value="" );


class Configuration {
    
public:
    void save( std::string config_file ) const;
    void load( std::string config_file );
    
    void validate ();
    void validate ( const YAML::Node & node );
    
    virtual void from_yaml( const YAML::Node & node ) = 0;
    virtual void to_yaml( YAML::Node & node ) const = 0;
};


template <typename C>
void load_config_file( std::string filename, C & config );


} // namespace configuration

#include "configuration.ipp"
