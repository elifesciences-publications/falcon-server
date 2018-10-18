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

#include <iostream>

#include "likelihooddata.hpp"
#include "utilities/string.hpp"
#include "utilities/math_numeric.hpp"

constexpr double LikelihoodData::DEFAULT_LOG_LIKELIHOOD_VALUE;
constexpr double LikelihoodData::DEFAULT_LIKELIHOOD_VALUE;

void LikelihoodData::Initialize ( size_t grid_size ) {
    
    grid_size_ = grid_size;
    this->ClearData();
}

size_t LikelihoodData::grid_size() const {
    
    return grid_size_;
}

unsigned int LikelihoodData::n_spikes() const {
    
    return n_spikes_;
}

void LikelihoodData::add_spikes(unsigned int nspikes) {
    
    n_spikes_ += nspikes;
}

double LikelihoodData::time_bin() const {
    
    return time_bin_ms_;
}

void LikelihoodData::set_time_bin( double buffer_size ) {
    
    time_bin_ms_ = buffer_size;
}

void LikelihoodData::set_log_likelihood(std::valarray<double>& log_likelihood) {
    
    log_likelihood_ = log_likelihood;
    likelihood_is_updated_ = false;
}

void LikelihoodData::set_log_likelihood(double log_likelihood_value, size_t grid_index) {
    
    log_likelihood_[grid_index] = log_likelihood_value;
    likelihood_is_updated_ = false;
}

void LikelihoodData::set_log_likelihood(double* log_likelihood_values, size_t grid_size) {
    
    for (decltype(grid_size) g = 0; g < grid_size; ++g) {
        // additional checks are fine because this method is not going to be used
        // in graphs dedicated to real-time processing 
        if ((log_likelihood_values + g) != nullptr) {
            log_likelihood_[g] = log_likelihood_values[g];
        } else {
            throw std::runtime_error("Invalid address at grid " + std::to_string(g));
        }
    }
    likelihood_is_updated_ = false;
    cached_integral_likelihood_ = INTEGRAL_OBSOLETE;
}

const std::valarray<double>& LikelihoodData::log_likelihood() const {
    
    return log_likelihood_;
}

double LikelihoodData::likelihood(std::size_t grid_index) {
    
    return likelihood()[grid_index];
}

void LikelihoodData::decrement_loglikelihood(double amount, size_t grid_index) {
    
    log_likelihood_[grid_index] -= amount;
    likelihood_is_updated_ = false;
    cached_integral_likelihood_ = INTEGRAL_OBSOLETE;
}

void LikelihoodData::increment_loglikelihood(double amount, size_t grid_index) {
    
    log_likelihood_[grid_index] += amount;
    likelihood_is_updated_ = false;
    cached_integral_likelihood_ = INTEGRAL_OBSOLETE;
}

const std::valarray<double>& LikelihoodData::likelihood() {
    
    if (!likelihood_is_updated_) {
        cached_likelihood_ = compute_likelihood( log_likelihood_ );
        likelihood_is_updated_ = true;
    }
    return cached_likelihood_;
}

std::valarray<double> LikelihoodData::compute_likelihood(
    std::valarray<double> log_likelihood ) const {
    
    std::valarray<double> normalized_log_likelihood =
        log_likelihood - log_likelihood.max();

    return std::exp( normalized_log_likelihood );
}

double LikelihoodData::integral_likelihood( bool nan_aware ) {
    
    if ( cached_integral_likelihood_ == INTEGRAL_OBSOLETE ) {
        if ( nan_aware ) {
            // small bug here: no track of whether the cached likelihood was
            // computed with or without nan awareness
            cached_integral_likelihood_ = nan_sum( std::begin(likelihood()),
                std::end(likelihood()) );
        } else {
            cached_integral_likelihood_ = likelihood().sum();
        }
    }
    return cached_integral_likelihood_;
}

void LikelihoodData::multiply_likelihood_inplace ( LikelihoodData* other,
    bool log_space ) {
    
    if (log_space) {
        log_likelihood_ += other->log_likelihood();
        likelihood_is_updated_ = false;
        cached_integral_likelihood_ = INTEGRAL_OBSOLETE;
    } else {
        cached_likelihood_ = this->likelihood() * other->likelihood();
        cached_integral_likelihood_ = INTEGRAL_OBSOLETE;
        // because we want always an up-to-date log_likelihood:
        log_likelihood_ = std::exp(cached_likelihood_);
    }
    this->add_spikes( other->n_spikes() );
}

void LikelihoodData::accumulate_likelihood( LikelihoodData* other,  bool log_space ) {
    
    this->multiply_likelihood_inplace(other, log_space);
    this->set_time_bin( this->time_bin() + other->time_bin() );
    this->add_spikes( other->n_spikes() );
}

std::size_t LikelihoodData::argmax() const {
    
    std::size_t argmax = 0, i;
    double value = log_likelihood_[argmax];
    double max_current_value = value;
    
    assert (grid_size_ == log_likelihood_.size());
    for (i=1; i < grid_size_; ++ i) {
        value = log_likelihood_[i];
        if ( value > max_current_value ) {
            max_current_value = value;
            argmax = i;
        }
    }
    return argmax;
}

double LikelihoodData::mua() const {
    
    return n_spikes_ / (time_bin_ms_ * 1e-3);
}

void LikelihoodData::ClearData() {
    
    n_spikes_ = 0;
    
    log_likelihood_.resize(grid_size_);
    log_likelihood_ = DEFAULT_LOG_LIKELIHOOD_VALUE;
    likelihood_is_updated_ = true; // default_likelihood = exp(default_log_likelihood)
    cached_likelihood_.resize(grid_size_);
    cached_likelihood_ = DEFAULT_LIKELIHOOD_VALUE;
    cached_integral_likelihood_ = cached_likelihood_.sum();
}

const double& LikelihoodData::operator[](std::size_t idx) const {
    
    return log_likelihood_[idx]; 
}

void LikelihoodData::YAMLDescription( YAML::Node & node,
    Serialization::Format format ) const {

    IData::YAMLDescription( node, format );
    
    if ( format==Serialization::Format::FULL ) {
        YAMLDescriptionCompact( node );
        node.push_back( LIKELIHOOD_S + " " + get_type_string<double>() +
        " (" + std::to_string(grid_size_) + ")" );
    }
    
    if ( format == Serialization::Format::COMPACT ) {
        YAMLDescriptionCompact( node );
    }
}

void LikelihoodData::YAMLDescriptionCompact( YAML::Node & node ) const {
    
    node.push_back( N_SPIKES_S + " " +
        get_type_string<decltype(n_spikes_)>() + " (1)" );
    node.push_back( TIME_BIN_S + " " +
        get_type_string<decltype(time_bin_ms_)>() + " (1)" );
    node.push_back( LOG_LIKELIHOOD_S + " " + get_type_string<double>() +
        " (" + std::to_string(grid_size_) + ")" );
}

void LikelihoodData::SerializeBinary( std::ostream& stream,
    Serialization::Format format ) const {

    IData::SerializeBinary( stream, format );
    
    if ( format==Serialization::Format::COMPACT ) {
        SerializeBinaryCompact( stream );
    }
    
    if ( format==Serialization::Format::FULL ) {
        // likelihood() method cannot be used because it will break the const-ness
        std::valarray<double> likelihood; 
        SerializeBinaryCompact( stream );
        if (!likelihood_is_updated_) {
            likelihood = compute_likelihood( log_likelihood_ );
        } else {
            likelihood = cached_likelihood_;
        }
        stream.write( reinterpret_cast<const char*>( &likelihood[0] ),
            sizeof( decltype( &likelihood[0] ) ) * grid_size_ );
    }
}

void LikelihoodData::SerializeYAML( YAML::Node & node,
    Serialization::Format format ) const {

    IData::SerializeYAML( node, format );
    
    if ( format==Serialization::Format::FULL || format==Serialization::Format::COMPACT ) {
        // valarrays are not accepted by YAML and must be converted into vectors
        std::vector<double> log_likelihood( log_likelihood_.size() );
        for ( size_t i=0; i < log_likelihood_.size(); ++i) {
            log_likelihood[i] = log_likelihood_[i];
        }
        node[N_SPIKES_S] =  n_spikes_;
        node[TIME_BIN_S] = time_bin_ms_;
        node[LOG_LIKELIHOOD_S] = log_likelihood;
    }
    
    if ( format==Serialization::Format::FULL ) {
        std::valarray<double> likelihood;
        if (!likelihood_is_updated_) {
            likelihood = compute_likelihood( log_likelihood_ );
        } else {
            likelihood = cached_likelihood_;
        }
        // valarrays are not accepted by YAML and must be converted into vectors
        std::vector<double> likelihood_v( likelihood.size() );
        for ( size_t i=0; i < likelihood.size(); ++i) {
            likelihood_v[i] = likelihood[i];
        }
        node[LIKELIHOOD_S] = likelihood_v;   
    }
}

void LikelihoodData::SerializeBinaryCompact( std::ostream& stream ) const {
    
    stream.write( reinterpret_cast<const char*>( &n_spikes_ ) ,
        sizeof( decltype(n_spikes_)) );
    stream.write( reinterpret_cast<const char*>( &time_bin_ms_ ) ,
        sizeof( decltype(time_bin_ms_)) );
    stream.write( reinterpret_cast<const char*>( &log_likelihood_[0] ),
        sizeof( decltype(log_likelihood_[0]) ) * grid_size_ );
}

double LikelihoodDataType::time_bin_ms() const {
    
    return time_bin_ms_;
}

size_t LikelihoodDataType::grid_size() const {
    
    return grid_size_;
}
    
void LikelihoodDataType::InitializeData( LikelihoodData& item ) const {
    
    item.Initialize( grid_size_ );
}
        
void LikelihoodDataType::Finalize( double time_bin, size_t grid_size ) {

    if ( time_bin <= 0 ) {
        throw std::runtime_error( "Bin size must be positive ");
    }
    time_bin_ms_ = time_bin;
    if ( grid_size == 0 ) {
        throw std::runtime_error( "Grid size must be different from zero");
    }
    if ( grid_size > HIGH_GRID_SIZE ) {
        throw std::runtime_error( "Grid size of " + std::to_string(grid_size) + " might be too high." );
       //     << std::endl;
    }
    grid_size_ = grid_size;
    AnyDataType::Finalize();
}

void LikelihoodDataType::Finalize( LikelihoodDataType& upstream ) {
        
    Finalize( upstream.time_bin_ms(), upstream.grid_size() );
}
