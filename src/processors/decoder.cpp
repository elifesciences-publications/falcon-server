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

#include "decoder.hpp"
#include "utilities/math_numeric.hpp"
#include "npyreader/npyreader.h"


void Decoder::Configure( const YAML::Node & node, const GlobalContext& context) {
    
    time_bin_ms_ = node["time_bin_ms"].as<double>( DEFAULT_BIN_SIZE );
    strict_time_bin_check_ = node["strict_time_bin_check"].as<bool> (
        DEFAULT_STRICT_TIME_BIN_CHECK );
    use_offline_model_ = node["use_offline_model"].as<bool>();
    if ( use_offline_model_ ) {
        path_to_offline_model_ = context.resolve_path(
            node["path_to_offline_model"].as<std::string>() );
        if ( path_to_offline_model_.back() != '/' ) {
            complete_path( path_to_offline_model_, name(), "" );
            path_to_offline_model_.pop_back(); // remove dot added by complete_path
            path_to_offline_model_.push_back('/');
        }
    } else {
        path_to_offline_model_ = node["path_to_offline_model"].as<std::string>(
            "no_path");
        LOG_IF( WARNING, ( path_to_offline_model_ != "no_path" ) ) << name() <<
            ". Entered path to offline model will be ignored.";
    }
    path_to_grid_ = context.resolve_path( node["path_to_grid"].as<std::string>() );
    if ( not path_exists( path_to_grid_ ) ) {
        throw ProcessingConfigureError("This file does not exit: " + path_to_grid_ );
    }    
    
    // small workaround to check if there is a path to test spikes
    // if there is, it is interpreted as a Falcon path and stored
    std::string possible_path;
    possible_path = node["path_to_test_spikes"].as<std::string>(NULL_PATH);
    if ( possible_path != NULL_PATH ) {
        path_to_test_spikes_ = context.resolve_path(
            node["path_to_test_spikes"].as<std::string>() );
    } else {
        path_to_test_spikes_ = NULL_PATH;
    }
}

void Decoder::CreatePorts( ) {
    
    data_in_port_ = create_input_port(
        "spikes",
        SpikeDataType( time_bin_ms_, ChannelRange( 1, MAX_N_AMPLITUDES ) ),
        PortInPolicy( SlotRange(1) ) );
    
    data_out_port_ = create_output_port(
        "estimates",
        LikelihoodDataType(),
        PortOutPolicy( SlotRange(1) ) );
    
    std::intptr_t none = 0;
    encoding_model_ = create_readable_shared_state(
        "encoding_model",
        none,
        Permission::WRITE,
        Permission::NONE );
}

void Decoder::CompleteStreamInfo() {
    
    // during negotiation it will be checked whether :
    // 1) the spike detector provides enough amplitudes
    // 2) that the detection has been carried out with a buffer-size
    //      not higher than the one set in the decoder and, 
    // 3) if smaller, that this is a integer multiple of the bin-size
    //      we want to decode with
    
    n_features_ = data_in_port_->slot(0)->streaminfo().datatype().n_channels();
    
    LOG(INFO) << name() << ". Using " <<  n_features_ << " spike features.";

    double spike_buffer_size =
        data_in_port_->slot(0)->streaminfo().datatype().buffer_size();
    
    check_buffer_sizes_and_log( spike_buffer_size,
        time_bin_ms_, strict_time_bin_check_, n_target_spikebuffers_, name() );
    
    n_grid_points_ = EncodingModel( n_features_, path_to_grid_ ).n_grid_elements();
    data_out_port_->streaminfo(0).datatype().Finalize(
            time_bin_ms_, n_grid_points_ );
    data_out_port_->streaminfo(0).Finalize(
        data_in_port_->streaminfo(0).stream_rate() / n_target_spikebuffers_ );
}

void Decoder::Prepare( GlobalContext& context ) {
    
    if ( use_offline_model_ ) {
        offline_model_ = new EncodingModel( n_features_, path_to_grid_ );
        offline_model_->from_disk( path_to_offline_model_ );
        LOG_IF(INFO, (offline_model_ != nullptr)) << name() <<
            ". Encoding model loaded successfully from "
            << path_to_offline_model_ << ".";
        
        LOG(DEBUG) << name() << ". N components : " << offline_model_->n_components_pax();
        LOG(DEBUG) << name() << ". N features model : " << offline_model_->n_features();
        assert( offline_model_->n_grid_elements() == n_grid_points_ );
        
//        mixture_update_cache( offline_model_->pax_model() );
    }
    
    dimensions_.clear();
    unsigned int i, ngrid_dim=1;
    for ( i=0; i < n_features_; i++ ) {
        dimensions_.push_back( ngrid_dim + i );
    }
    
    LOG(DEBUG) << name() << ". n_target_spikebuffers_ = " << n_target_spikebuffers_;
    
    // open the NYP files and extract the number of spikes, making sure that
    // they are multiple of the number of spike amplitudes
    use_test_amplitudes_ = (path_to_test_spikes_ != NULL_PATH);
    if ( use_test_amplitudes_ ) {
        if ( path_to_test_spikes_.compare( path_to_test_spikes_.size()-4, 4, ".npy" ) != 0 ) { // not a NPY file
            complete_path( path_to_test_spikes_, name(), "npy" );            
        }
    }
    if ( use_test_amplitudes_ ) { 
        uint32_t n_test_amplitudes = 0;
        if ( !path_exists(path_to_test_spikes_) ) {
            throw std::runtime_error(path_to_test_spikes_ + " doesn't exist.");
        }
        LOG(INFO) << name() << ". Path to test spikes exists.";
        if ( (fp_test_amplitudes_ = fopen(path_to_test_spikes_.c_str(), "r")) == nullptr ) {
            throw ProcessingPrepareError("Cannot open the NPY file.", name());
        }
        LOG(INFO) << name() << ". NPY file opened correctly";
        if ( get_1D_array_len(fp_test_amplitudes_, &n_test_amplitudes) != 0 ) {
            throw ProcessingPrepareError(
            "Cannot read the length of the spike amplitudes array from the NPY file.",
            name());
        }
        LOG(INFO) << name() << ". " << n_test_amplitudes << " test amplitudes read correctly.";
        if ( n_test_amplitudes == 0 ) {
            throw ProcessingPrepareError(
            "Number of amplitudes must be greater than zero.", name());
        }
        if ( n_test_amplitudes % n_features_ != 0 ) {
            throw ProcessingPrepareError(
                "The number of test amplitudes is not a multiple of " +
                std::string("the number of spike amplitudes"), name());
        }
        n_test_spikes_ = n_test_amplitudes / n_features_;
        if ( (loaded_spike_amplitudes_ = get_1D_array_f64( fp_test_amplitudes_ ) )
        == nullptr ) {
            throw ProcessingPrepareError(
                "Test spike amplitudes were not loaded correctly.", name());
        }
        
        LOG(INFO) << name() << ". " << n_test_spikes_ << " test spikes were loaded. "
            << n_test_amplitudes << " test amplitudes were "
            << "loaded and will be used to generate the output likelihood"
            << ". Incoming amplitudes will be discarded.";
    }
}

void Decoder::Preprocess(ProcessingContext& context) {
    
    test_spike_amplitudes_ptr_ = loaded_spike_amplitudes_;
    n_used_spikes_ = 0;
}

void Decoder::Process( ProcessingContext& context ) {
    
    SpikeData* data_in = nullptr;
    LikelihoodData* data_out = nullptr;
    unsigned int n_spike_buffers = 0, n_spikes = 0;
    double time_bin = time_bin_ms_ / 1000; // decoding uses seconds
    unsigned int s, g;

//    EncodingModel* model = get_model();
    assert( n_grid_points_ == get_model()->n_grid_elements() );
    
    while ( !context.terminated() ) {
        
        // retrieve SpikeData and extract the number of spikes
        if (!data_in_port_->slot(0)->RetrieveData( data_in )) {break;}
        n_spikes = data_in->n_detected_spikes();
        
        // prepare Likelihood Data item with TS info
        if ( n_spike_buffers == 0 ) {
            data_out = data_out_port_->slot(0)->ClaimData( true );
            data_out->set_hardware_timestamp( data_in->hardware_timestamp() );
        }
        
        // if there are spikes process their contribution to the likelihood
        if ( n_spikes > 0 ) {
            data_out->add_spikes( n_spikes );
            // compute spike-amplitude contribution of the likelihood
            pax_.assign( n_spikes * n_grid_points_, 0 ); // reset pax
            if ( get_model()->n_components_pax() > 0 ) {
                if ( use_test_amplitudes_ ) { // testing mode with loaded test amplitudes
                    evaluate_test_amplitudes( get_model(), n_spikes );
                } else {
                    assert( get_model() != nullptr );
                    mixture_evaluategrid_diagonal_multi(
                        get_model()->pax_model(),
                        get_model()->accumulator().data(),
                        n_grid_points_,
                        data_in->amplitudes().data(),
                        n_spikes,
                        n_features_,
                        dimensions_.data(),
                        pax_.data() );  
                }
            }
            
            // add spike-amplitude contribution of the likelihood
            for ( s=0; s < n_spikes; ++s ) {
                for ( g=0; g < n_grid_points_; ++ g ) {
                    data_out->increment_loglikelihood(
                        std::log( pax_[s * n_grid_points_ + g] +
                        get_model()->offset() * get_model()->pix(g) / get_model()->mu() ), g );
                }
            }    
        }
        
        // increment counter of spike buffer and release retrieved Spike Data
        ++ n_spike_buffers;
        data_in_port_->slot(0)->ReleaseData();
        
        // when enough spike buffers have been retrieved, proceed to complete
        // the estimation of the likelihood and publish the data item
        if ( n_spike_buffers == n_target_spikebuffers_ ) {
                    
            // subtract spike-amplitude-independent contribution of the likelihood
            for (unsigned int g=0; g< n_grid_points_; ++ g) {        
                //can this be done before evaluating the pax?
                data_out->decrement_loglikelihood(
                    data_out->n_spikes() * std::log( get_model()->pix(g) )
                    + time_bin * get_model()->lambda_x(g), g );
            } 
            
            // update debug-mode only
            LOG_IF(DEBUG, (data_out_port_->slot(0)->nitems_produced()%200 == 0))
                << name() << ". Processed bin number: " <<
                data_out_port_->slot(0)->nitems_produced() << " (using " <<
                data_out->n_spikes() << " spikes).";
            
            // complete LikelihoodData item, publish it and reset spike buffer counter
            data_out->set_source_timestamp();
            data_out->set_time_bin( time_bin_ms_ );
            data_out_port_->slot(0)->PublishData();
            n_spike_buffers = 0;
            
            // update model (regulate frequency of checks for new model?)
            //delete model; model = nullptr;
//            model = get_model();
            LOG_IF(DEBUG, (data_out_port_->slot(0)->nitems_produced()%500 == 0))
                << name() << ". Model has " << get_model()->n_components_pax()
                << " components.";
        }
    }
}

void Decoder::Postprocess( ProcessingContext& context ) {

    LOG(INFO) << name() << ". " << data_out_port_->slot(0)->nitems_produced()
        << " likelihoods computed.";
}

void Decoder::Unprepare( GlobalContext& context ) {
    
//    delete initial_encoding_model_->amplitude_range_;
//    initial_encoding_model_->amplitude_range_ = nullptr;
//    delete_model(initial_encoding_model_); initial_encoding_model_ = nullptr;
    if ( loaded_spike_amplitudes_ != nullptr ) {
        free(loaded_spike_amplitudes_); loaded_spike_amplitudes_ = nullptr;
    }
    if ( use_offline_model_ ) {
        delete offline_model_; offline_model_ = nullptr;
    }
}

void Decoder::evaluate_test_amplitudes( EncodingModel* model,
unsigned int n_spikes ) {

    if ( n_used_spikes_ >= n_test_spikes_ ) {
        test_spike_amplitudes_ptr_ = loaded_spike_amplitudes_;
        n_used_spikes_ = 0;
    }

    mixture_evaluategrid_diagonal_multi(
        model->pax_model(),
        model->accumulator().data(),
        model->n_grid_elements(),
        test_spike_amplitudes_ptr_,
        n_spikes,
        n_features_,
        &dimensions_[0],
        &pax_[0]); 
    
    test_spike_amplitudes_ptr_ += (n_spikes * n_features_);
    n_used_spikes_ += n_spikes;
}
