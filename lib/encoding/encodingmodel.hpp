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

#ifndef ENCODINGMODEL_HPP
#define	ENCODINGMODEL_HPP

#include <vector>

#include "environment/environment.hpp"
#include "gmm/mixture.h"
#include "utilities/math_numeric.hpp"

typedef Range<unsigned int> AmplitudeRange;

static const auto EXTENSION_PRECOMPUTATION = ".npy";
static const auto EXTENSION_GMM = ".dat";
static const auto EXTENSION_OFFSET = EXTENSION_PRECOMPUTATION;

const std::string ENCODING_MODEL_S = "encoding_model";

const double DEFAULT_OFFSET = 1e-9;

class EncodingPoint {
    
public:
    EncodingPoint( std::size_t dim_spikefeatures );
    EncodingPoint( std::size_t dim_spikefeatures, double behavior,
        std::vector<double>::const_iterator ampl_it );
    
    void set_gmm_point( double behavior, std::vector<double>::const_iterator ampl_it );
    std::vector<double> gmm_point() const;
    
    void set_timestamp( uint64_t timestamp );
    uint64_t timestamp() const;
            
private:
    std::size_t dim_spikefeatures_;
    double behavior_;
    std::vector<double> spike_features_;
    std::vector<double> gmm_point_;
    uint64_t timestamp_;
};


class EncodingModel {
    
public:
    EncodingModel( std::size_t n_features, std::string path_to_grid,
        double bw_behav=1.0, double bw_spike_features=1.0, double offset=DEFAULT_OFFSET,
        unsigned int serial_number=0 );
    EncodingModel( const EncodingModel& copy );
    ~EncodingModel();
    
    EncodingModel* clone();
    
    double bw_behav() const;
    double bw_spike_amplitudes() const;
    
    void create_accumulator_pax();
    
    double mu() const;
    void set_mu( double mu );
    
    void load_pax_fromfile( std::string path_to_paxmodel );
    
    void set_pix( std::vector<double> pix );
    double pix( std::size_t grid_index ) const;
    
    double px( std::size_t grid_index ) const;
    std::vector<double> px() const;
    void compute_px_model();
    
    void set_lambda_x( std::vector<double> lambda_x );
    void compute_lambda_x();
    std::vector<double> lambda_x();
    double lambda_x( std::size_t grid_index );
    
    uint16_t n_features() const;
    
    void merge_point( EncodingPoint& point, double threshold_compression );
    void compute_marginal();
    
    std::vector<double>& accumulator();
    
    void evaluate_px();
    Mixture* px_model();
    unsigned int n_components_px() const;
    
    Mixture* pax_model();
    unsigned int n_components_pax() const;
    
    unsigned int n_grid_elements() const;
    double grid_value( std::size_t grid_index ) const;
    
    double offset() const;
    void set_offset( double offset );
    
    unsigned int serial_number() const;
    void increase_serial_number();
    
    void to_disk( std::string common_path, std::string processor_name,
        std::string model_filename_extension ="dat" );
    void from_disk( std::string path_to_model );
    
protected:
    void set_covars();

protected:
    uint8_t n_spike_features_;
    
    std::string path_to_grid_;
    std::unique_ptr<Grid1D> grid_;
    
    Mixture* pax_model_;
    Mixture* px_model_;
    double bw_behav_;
    double bw_spike_features_;
    std::vector<double> pix_;
    std::vector<double> px_;
    std::vector<double> lambda_x_;
    double mu_;
    bool precomputations_updated_; // precomputation of lambda_x
    double offset_;
    
    std::vector<double> accumulator_;
    
    unsigned int serial_number_;
    
    std::array<uint16_t, 1> behavior_array_;
};

#endif	// encodingmodel.hpp

