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

#ifndef LIKELIHOODDATA_HPP
#define	LIKELIHOODDATA_HPP

#include "idata.hpp"
#include <valarray> 


class LikelihoodData : public IData {
public:
    void Initialize( size_t grid_size );
    
    size_t grid_size() const;
    
    unsigned int n_spikes() const;
    void add_spikes(unsigned int nspikes);
    
    /* returns decoding bin [ms] */
    double time_bin() const;
    void set_time_bin( double buffer_size );
    
    void set_log_likelihood(std::valarray<double>& log_likelihood); // copy
    void set_log_likelihood(double log_likelihood_value, size_t grid_index);
    void set_log_likelihood(double* log_likelihood_values, size_t grid_size);
    
    const std::valarray<double>& log_likelihood() const;
    const double& operator[](std::size_t idx) const; // no [] access to likelihood in regular space; remove it if creates confusion
    
    double likelihood(std::size_t grid_index);
    const std::valarray<double>& likelihood();
    
    double integral_likelihood( bool nan_aware = false );
    
    void decrement_loglikelihood(double amount, size_t grid_index);
    void increment_loglikelihood(double amount, size_t grid_index);
    
    void multiply_likelihood_inplace( LikelihoodData* other, bool log_space = true );
    
    void accumulate_likelihood( LikelihoodData* other, bool log_space = true );
    
    std::size_t argmax() const;
    
    double mua() const;
    
    virtual void ClearData() override;
    
    void YAMLDescription( YAML::Node & node,
        Serialization::Format format = Serialization::Format::FULL ) const override;
    
    void SerializeBinary( std::ostream& stream,
        Serialization::Format format = Serialization::Format::FULL) const override;
    
    void SerializeYAML( YAML::Node & node,
        Serialization::Format format = Serialization::Format::FULL ) const override;
    
protected:
    void YAMLDescriptionCompact( YAML::Node & node ) const;
    void SerializeBinaryCompact( std::ostream& stream ) const;
    
public:
    static constexpr double DEFAULT_LOG_LIKELIHOOD_VALUE = 0;
    static constexpr double DEFAULT_LIKELIHOOD_VALUE =
        std::exp(DEFAULT_LOG_LIKELIHOOD_VALUE);
    
protected:
    std::valarray<double> compute_likelihood( std::valarray<double> log_likelihood ) const;
    std::valarray<double> normalized_log_likelihood();
    
protected:
    std::valarray<double> log_likelihood_;
    bool likelihood_is_updated_;
    std::valarray<double> cached_likelihood_;
    double cached_integral_likelihood_;
    size_t grid_size_; // # points of the 1-D decoding grid
    unsigned int n_spikes_; // number of spikes that were used to compute the likelihood
    double time_bin_ms_; // duration (in ms) of the buffer of data used to compute the likelihood

protected:
    const std::string N_SPIKES_S = "n_spikes";
    const std::string TIME_BIN_S = "time_bin_ms";
    const std::string LOG_LIKELIHOOD_S = "log_likelihood";
    const std::string LIKELIHOOD_S = "likelihood";
    const double INTEGRAL_OBSOLETE = -1;
};


class LikelihoodDataType : public AnyDataType {

ASSOCIATED_DATACLASS(LikelihoodData);

public:
    LikelihoodDataType( size_t grid_size = DEFAULT_GRID_SIZE ) : grid_size_(grid_size) {}
    
    virtual void InitializeData( LikelihoodData& item ) const;
    
    size_t grid_size() const;
    
    double time_bin_ms() const;
    
    virtual void Finalize( double time_bin, size_t grid_size );

    virtual void Finalize( LikelihoodDataType& upstream );
    
    static const size_t DEFAULT_GRID_SIZE = 1;
    const size_t HIGH_GRID_SIZE = 1000;
    
    virtual std::string name() const { return "likelihood"; }
          
protected:
    size_t grid_size_;
    double time_bin_ms_;
};

#endif	/// likelihooddata.hpp
