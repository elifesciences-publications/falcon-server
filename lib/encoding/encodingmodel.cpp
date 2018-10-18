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

#include "encodingmodel.hpp"

#include "environment/environment.hpp"
#include "npyreader/npyreader.h"
#include "utilities/string.hpp"
#include "gmm/mixture.h"
#include "cnpy/cnpy.h"

#include <cassert>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <algorithm>

#include <math.h> 
#include <sys/stat.h>


EncodingPoint::EncodingPoint( std::size_t dim_spikefeatures ) {
    
    dim_spikefeatures_ = dim_spikefeatures;
    spike_features_.resize( dim_spikefeatures_ );
    gmm_point_.resize( dim_spikefeatures_ + 1);
}

EncodingPoint::EncodingPoint( std::size_t dim_spikefeatures, double behavior,
std::vector<double>::const_iterator ampl_it ) {
    
    dim_spikefeatures_ = dim_spikefeatures;
    spike_features_.resize( dim_spikefeatures_ );
    gmm_point_.resize( dim_spikefeatures_ + 1);
    set_gmm_point( behavior, ampl_it );
}

void EncodingPoint::set_gmm_point( double behavior, std::vector<double>::const_iterator ampl_it ) {
    
    gmm_point_[0] = behavior;
    auto j = 1;
    for ( auto it=ampl_it; it!=(ampl_it+dim_spikefeatures_); ++it ) {
        gmm_point_[j] = *it;
        ++ j;
    }
}

std::vector<double> EncodingPoint::gmm_point() const {
    
    return gmm_point_;
}

void EncodingPoint::set_timestamp( uint64_t timestamp ) {
    
    timestamp_ = timestamp;
}

uint64_t EncodingPoint::timestamp() const {
    
    return timestamp_;
}

EncodingModel::EncodingModel( std::size_t n_features, std::string path_to_grid,
double bw_behav, double bw_spike_features, double offset, unsigned int serial_number ) {
    
    n_spike_features_ = n_features;
    
    path_to_grid_ = path_to_grid;
    grid_.reset( new Grid1D( path_to_grid ) );
    
    pax_model_ = mixture_create( 1 + n_features );
    px_model_ = mixture_create( 1 ); 
    
    bw_behav_ = bw_behav;
    bw_spike_features_ = bw_spike_features;
    offset_ = offset;
    
    pix_.assign( grid_->n_elements(), 0 );
    px_.assign( grid_->n_elements(), 0 );
    lambda_x_.assign( grid_->n_elements(), 0 );
    mu_ = 0;
    
    precomputations_updated_ = false;
    
    set_covars();
    
    serial_number_ = serial_number;
    
    behavior_array_.fill( 0 );
}

EncodingModel::EncodingModel( EncodingModel const& input ) {
    
    n_spike_features_ = input.n_spike_features_;
    
    path_to_grid_ = input.path_to_grid_;
    grid_.reset( new Grid1D( input.path_to_grid_ ) );
    
    bw_behav_ = input.bw_behav_;
    bw_spike_features_ = input.bw_spike_features_;
    offset_ = input.offset_;

    pax_model_ = copy_mixture( input.pax_model_ );
    px_model_ = copy_mixture( input.px_model_ );
    accumulator_ = input.accumulator_;
    set_covars();

    pix_ = input.pix_;
    px_ = input.px_;
    lambda_x_ = input.lambda_x_;
    mu_ = input.mu_;

    precomputations_updated_ = input.precomputations_updated_;
    
    serial_number_ = input.serial_number_;
    
    behavior_array_.fill( 0 );
}

EncodingModel::~EncodingModel() {
    
    assert ( px_model_ != nullptr );
    assert ( pax_model_ != nullptr );
    
    mixture_delete(pax_model_); pax_model_ = nullptr;
    mixture_delete(px_model_); px_model_ = nullptr;
}

EncodingModel* EncodingModel::clone() {
    
    return new EncodingModel( *this );
}
    
double EncodingModel::bw_behav() const {

    return bw_behav_;
}
double EncodingModel::bw_spike_amplitudes() const {
    
    return bw_spike_features_;
}

void EncodingModel::create_accumulator_pax() {
    
    assert( pax_model_ != nullptr );
    
    accumulator_.resize( pax_model_->ncomponents * grid_->n_elements() );
    mixture_prepare_grid_accumulator( pax_model_, grid_->elements(),
        grid_->n_elements(), 1, grid_->dimensions(), accumulator_.data() );
}

double EncodingModel::mu() const {
    
    return mu_;
}

void EncodingModel::set_mu( double mu ) {
    
    mu_ = mu;
    precomputations_updated_ = false;
}

void EncodingModel::set_pix( std::vector<double> pix ) {
    
    assert( grid_->n_elements() == pix.size() );
    pix_ = pix;
}

void EncodingModel::load_pax_fromfile( std::string path_to_paxmodel ) {
    
    pax_model_ = mixture_load_from_file( path_to_paxmodel.c_str() );
    if ( n_spike_features_ != ( pax_model_->ndim - 1 ) ) {
        throw std::runtime_error(
            "NUmber of features of the loaded model is inconsistent");
    }
}

void EncodingModel::set_lambda_x( std::vector<double> lambda_x ) {
    
    assert( grid_->n_elements() == lambda_x.size() );
    lambda_x_ = lambda_x; 
}

double EncodingModel::pix( std::size_t grid_index ) const {
    
    if ( grid_index >= pix_.size() ) {
        std::cout << " grid index = " << grid_index << "; pix_.size() = " << pix_.size() << std::endl;
    }
    assert( grid_index < pix_.size() );
    return pix_[grid_index];
}

double EncodingModel::px( std::size_t grid_index ) const {
    
    assert( grid_index <= px_.size() );
    return px_[grid_index];
}

std::vector<double> EncodingModel::px() const {
    
    return px_;
}

void EncodingModel::compute_px_model() {
    
    mixture_delete( px_model_); px_model_ = nullptr;
    px_model_ = mixture_marginalize( pax_model_, 1, behavior_array_.data() );
}

std::vector<double> EncodingModel::lambda_x() {
    
    if ( not precomputations_updated_ ) {
        compute_lambda_x();
    }
    return lambda_x_;
}

double EncodingModel::lambda_x( std::size_t grid_index ) {
    
    if ( not precomputations_updated_ ) {
        compute_lambda_x();
    }
    assert( grid_index < lambda_x_.size() );
    return lambda_x_[grid_index];
}

uint16_t EncodingModel::n_features() const {
    
    return n_spike_features_;
}

void EncodingModel::merge_point( EncodingPoint& point, double threshold_compression ) {
    
    mixture_merge_samples_match_bandwidth(
        pax_model_,
        point.gmm_point().data(),
        1,
        threshold_compression);
}

void EncodingModel::compute_marginal() {
    
    assert( pax_model_ != nullptr );
    px_model_ = mixture_marginalize( pax_model_, 1, behavior_array_.data() );
}
    
std::vector<double>& EncodingModel::accumulator() {
    
    return accumulator_;
}

Mixture* EncodingModel::px_model() {
    
    return px_model_;
}

unsigned int EncodingModel::n_components_px() const {
    
    return px_model_->ncomponents;
}

Mixture* EncodingModel::pax_model() {
    
    return pax_model_;
}

unsigned int EncodingModel::n_components_pax() const {
    
    return pax_model_->ncomponents;
}

unsigned int EncodingModel::n_grid_elements() const {
    
    return grid_->n_elements();
}
    
double EncodingModel::grid_value( std::size_t grid_index ) const {
    
    return grid_->element( grid_index );
}

double EncodingModel::offset() const {
    
    return offset_;
}

void EncodingModel::set_offset( double offset ) {
    
    offset_ = offset;
}

unsigned int EncodingModel::serial_number() const {
    
    return serial_number_;
}

void EncodingModel::increase_serial_number() {
    
    ++ serial_number_;
}

void to_disk( std::string common_path, std::string processor_name,
    std::string model_filename_extension="dat" );


void EncodingModel::set_covars() {
    
    std::vector<double> pax_covars;
    pax_covars.push_back( std::pow( bw_behav_, 2 ) );
    
    for ( unsigned int i=0; i<n_spike_features_; ++i ) {
        pax_covars.push_back( std::pow( bw_spike_features_, 2 ) );
    }
    
    CovarianceMatrix *cov_matrix = covariance_create_zero( 1+n_spike_features_ );
    covariance_set_diagonal( cov_matrix, pax_covars.data() );
    
    mixture_set_samplecovariance( pax_model_, cov_matrix );
    covariance_delete( cov_matrix ); cov_matrix = nullptr;
}

void EncodingModel::evaluate_px() {
    
    if ( grid_->n_elements() == 0 ) { throw std::runtime_error("Grid is empty");}
    
    assert ( px_model_ != nullptr );

    if ( px_model_->ncomponents != 0 ) {
        mixture_evaluate( px_model_, grid_->elements(), grid_->n_elements(), px_.data() );
    } else {
        for ( unsigned int i=0; i<grid_->n_elements(); ++i) {
            px_[i] = 0;
        }
    }
    
    precomputations_updated_ = false;
}

void EncodingModel::compute_lambda_x() {
    
    for ( unsigned int i=0; i<grid_->n_elements(); ++i ) {
        lambda_x_[i] = mu_ * px_[i] / pix_[i] + DEFAULT_OFFSET;
    }
    
    precomputations_updated_ = true;
}

void EncodingModel::to_disk( std::string common_path, std::string processor_name,
std::string model_filename_extension ) {

    assert( pax_model_ != nullptr );
    
    auto dotted_filename_extension = "." + model_filename_extension;
    unsigned int npyshape_lambda_x = lambda_x_.size();
    decltype(npyshape_lambda_x) npyshape_singledouble = 1;
    
    auto path = common_path + "/" + processor_name;
    if ( not path_exists( path ) ) {
        mkdir( path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );
    }
    
    auto serials_filename = path + "/serialnumber.txt";
    std::ofstream serials_file (serials_filename);
    
    mixture_save_to_file( this->pax_model_,
        ( path + "/mixture_pax" +
        dotted_filename_extension).c_str() );
    mixture_save_to_file( this->px_model_,
        ( path + "/px" +
        dotted_filename_extension).c_str() );
    
    if ( not precomputations_updated_ ) {
        compute_lambda_x();
    }
    cnpy::npy_save( path + "/lambda_x.npy",
        lambda_x().data(), &npyshape_lambda_x, 1, "w" );
    cnpy::npy_save( path + "/pix.npy",
        pix_.data(), &npyshape_lambda_x, 1, "w" );
    cnpy::npy_save( path + "/px.npy",
        px().data(), &npyshape_lambda_x, 1, "w" );
    auto mu = this->mu();
    cnpy::npy_save( path + "/mu.npy",
        &mu, &npyshape_singledouble, 1, "w" );
    auto offset = this->offset();
    cnpy::npy_save( path + "/offset.npy",
        &offset, &npyshape_singledouble, 1, "w" );
    
    serials_file << this->serial_number_;
    serials_file.close();
} 

void EncodingModel::from_disk( std::string path_to_model ) {
    
    // does NOT load px!
    
    std::string pix_path;
    std::string lx_path;
    std::string mu_path;
    std::string mixture_pax_path;
    std::string offset_path;
            
    pix_path = extract_path_to_folder( path_to_model ) +
        "pix" + EXTENSION_PRECOMPUTATION;
    if (!path_exists(pix_path)) {
        throw std::runtime_error("This path doesn't exist: " + pix_path);
    }
    lx_path = path_to_model + "lambda_x" + EXTENSION_PRECOMPUTATION;
    if (!path_exists(lx_path)) {
        throw std::runtime_error("This path doesn't exist: " + lx_path);
    }
    mu_path = path_to_model + "mu" + EXTENSION_PRECOMPUTATION;
    if (!path_exists(mu_path)) {
        throw std::runtime_error("This path doesn't exist: " + mu_path);
    }
    mixture_pax_path = path_to_model + "mixture_pax" + EXTENSION_GMM;
    if (!path_exists(mixture_pax_path)) {
        throw std::runtime_error("This path doesn't exist: " + mixture_pax_path);
    }
    offset_path = path_to_model + "offset" + EXTENSION_OFFSET;
    if (!path_exists(offset_path)) {
        throw std::runtime_error("This path doesn't exist: " + offset_path);
    }

    // open files within the subfolder paths
    FILE *pix_file = nullptr;
    FILE *lx_file = nullptr;
    FILE *mu_file = nullptr;
    FILE *offset_file = nullptr;
    if ( (pix_file = fopen(pix_path.c_str(), "r")) == nullptr ) {
        throw std::runtime_error("Cannot open the pix file!");
    }
    if ( (lx_file = fopen(lx_path.c_str(), "r")) == nullptr ) {
        throw std::runtime_error("Cannot open the lx file!");
    } 
    if ( (mu_file = fopen(mu_path.c_str(), "r")) == nullptr ) {
        throw std::runtime_error("Cannot open the mu file!");
    }
    if ( (offset_file = fopen(offset_path.c_str(), "r")) == nullptr ) {
        throw std::runtime_error("Cannot open the offset file!");
    }

    double* pix_ptr = get_1D_array_f64(pix_file);
    uint32_t len_pix = 0;
    get_1D_array_len(pix_file, &len_pix);
    assert ( len_pix == n_grid_elements() );
    std::vector<double> pix( pix_ptr, pix_ptr + n_grid_elements() );
    set_pix( pix );

    double* lx_ptr = get_1D_array_f64(lx_file);
    uint32_t len_lx = 0;
    get_1D_array_len(lx_file, &len_lx);
    assert ( len_lx == n_grid_elements() );
    std::vector<double> lambda_x( lx_ptr, lx_ptr +  n_grid_elements() );
    set_lambda_x( lambda_x );
    
    set_mu( *retrieve_npy_float64(mu_file) );
    
    set_offset( *retrieve_npy_float64(offset_file) );
    
    // close files and clean up pointers
    fclose(pix_file); pix_file = nullptr;
    fclose(lx_file); lx_file = nullptr;
    fclose(mu_file); mu_file = nullptr;
    fclose(offset_file); offset_file = nullptr;
    free(pix_ptr); pix_ptr = nullptr;
    free(lx_ptr); lx_ptr = nullptr;
    
    load_pax_fromfile( mixture_pax_path );
    
    create_accumulator_pax();
    
    precomputations_updated_ = true; // everything up-to-date
}
