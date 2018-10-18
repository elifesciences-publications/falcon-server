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

#include "behaviorreader.hpp"

#include "neuralynx/nlx.hpp"
#include "utilities/string.hpp"
#include "npyreader/npyreader.h"
#include "g3log/src/g2log.hpp"

void BehaviorReader::Configure(const YAML::Node& node, const GlobalContext& context) {
    
    path_to_linearization_matrix_ = node["path_to_linearization_matrix"].as<std::string>( );
    if ( not path_exists( path_to_linearization_matrix_ ) ) {
        use_single_coordinate_ =
            node["use_single_coordinate"].as<decltype(use_single_coordinate_)>(
                DEFAULT_USE_SINGLE_COORDINATE );
        if ( use_single_coordinate_ == 'x' ) {
            linearization_mode_ = LinearizationMode::X;
            LOG(INFO) << name() << ". Trivial linearization with X coordinate.";
        } else if ( use_single_coordinate_ == 'y' ) {
            linearization_mode_ = LinearizationMode::Y;
            LOG(INFO) << name() << ". Trivial linearization with Y coordinate.";
        } else {
            auto err_msg = "Path to linearization matrix does not exist (" +
                path_to_linearization_matrix_ +
                "). Enter path or choose a coordinate for trivial linearization";
            throw ProcessingConfigureError( err_msg, name() );
        }
    } else {
        linearization_mode_ = LinearizationMode::MATRIX;
        LOG(INFO) << name() << ". Linearization with matrix loaded from " <<
            path_to_linearization_matrix_ << ".";
    }
    
    pixel_to_cm_ = node["pixel_to_cm"].as<double>( -99 );
    if ( pixel_to_cm_ <= 0 ) {
        LOG(INFO) << name() << ". Behavior unit is " << PIXEL_S << ".";
    } else {
        LOG(INFO) << name() << ". Behavior unit is " << CM_S << ".";
    }
    
    n_ma_position_ = node["n_ma_position"].as<decltype(n_ma_position_)>(
        DEFAUL_N_MA_POSITION );
    n_ma_speed_ = node["n_ma_speed"].as<decltype(n_ma_speed_)>(
        DEFAULT_N_MA_SPEED );
    batch_size_ = node["batch_size"].as<decltype(batch_size_)>(
        DEFAULT_BATCH_SIZE );
    
    // set frequency of updates
    auto value = node["update_interval"].as<decltype(
        update_interval_)>( DEFAULT_UPDATE_INTERVAL_SEC );
    if ( value==0 ) {
        update_interval_ = std::numeric_limits<decltype(update_interval_)>::max();
    } else {
        update_interval_ = value * NLX_VIDEO_SAMPLING_FREQUENCY;
    }
}

void BehaviorReader::CreatePorts() {
    
    data_in_port_ = create_input_port(
        VIDEOTRACKDATA_S,
        // these values can be replaced with a user input
        VideoTrackDataType( NLX_VIDEO_SAMPLING_FREQUENCY, NLX_VIDEO_RESOLUTION ),
        PortInPolicy( SlotRange(1) ) );

    data_out_port_ = create_output_port(
        "behavior",
        BehaviorDataType(),
        PortOutPolicy( SlotRange(1) ) );

}

void BehaviorReader::CompleteStreamInfo( ) {
    
    if ( pixel_to_cm_ <= 0 ) {
        data_out_port_->streaminfo(0).datatype().Finalize( BehaviorUnit::PIXEL, 
            data_in_port_->slot(0)->streaminfo().datatype().sample_rate() );
    } else {
        data_out_port_->streaminfo(0).datatype().Finalize( BehaviorUnit::CM,
            data_in_port_->slot(0)->streaminfo().datatype().sample_rate());
    }
    data_out_port_->streaminfo(0).Finalize( data_in_port_->streaminfo(0).stream_rate() );
    
}

void BehaviorReader::Prepare( GlobalContext& context ) {
    
    if ( linearization_mode_ == LinearizationMode::MATRIX ) {
        // open the linearization matrix file
        FILE* fp_linearization_matrix = nullptr;
        if ( (fp_linearization_matrix =
        fopen(path_to_linearization_matrix_.c_str(), "r")) == NULL ) {
            auto err_msg = "Cannot open the NPY file. Check the file path " +
                path_to_linearization_matrix_;
            throw ProcessingPrepareError( err_msg, name() );
        }
    
        // check whether the linearization matrix is valid or not
        uint32_t n_rows = 0;
        uint32_t n_cols = 0;
        if ( get_2D_matrix_shape( fp_linearization_matrix, &n_rows, &n_cols ) != 0 ) {
            throw ProcessingPrepareError( 
                "Cannot read the size of the NPY file", name());
        }
        unsigned int expected_n_rows = data_in_port_->slot(0)->streaminfo().datatype().resolution()[0];
        unsigned int expected_n_cols = data_in_port_->slot(0)->streaminfo().datatype().resolution()[1];
        if ( (n_rows != expected_n_rows) or (n_cols != expected_n_cols) ) {
            throw ProcessingPrepareError( 
                "Linearization matrix has unexpected dimensions", name() );
        }

        // load the  matrix
        if ( (linearization_matrix_ = get_2D_matrix_f64( fp_linearization_matrix )) == NULL ) {
            throw ProcessingPrepareError(
                "Cannot load the likelihood data from the NPY file", name());
        }
        LOG(INFO) << name() << ". Linearization matrix was loaded correctly from file.";
        fclose(fp_linearization_matrix); fp_linearization_matrix = nullptr;
    }
    
    // set conversion factor
    conversion_factor_ = 1;
    if ( pixel_to_cm_ > 0 ) {
        conversion_factor_ = pixel_to_cm_;
    }
    
    
}

void BehaviorReader::Preprocess( ProcessingContext& context ) {
    
    double dt = 1 / data_in_port_->streaminfo(0).datatype().sample_rate();
    LOG(UPDATE) << name() << ". Behavior sampling period = " << dt*1e3 << " ms.";
    try {
        speed_calculator_.reset( new dsp::behavior_algorithms::SpeedCalculator( 
            n_ma_position_, n_ma_speed_, dt, batch_size_ ) );
    } catch (std::runtime_error e) {
        throw ProcessingPreprocessingError( e.what(), name() );
    }
    
    LOG(DEBUG) << name() << ". Delay will be offset by " << speed_calculator_->total_delay()
        << " samples ( position delay = " << speed_calculator_->delay_position()
        << " , speed delay = " << speed_calculator_->delay_speed() << " ).";
    
    // create files for intermediate variables in test mode
    // (does not work if moved to Prepare:  No storage context "run" exists.)
    if ( context.test() ) {  
        std::string path = context.resolve_path( "run://", "run" ) + "test/";
        auto prefix = path + name();
        create_file( prefix, SMOOTH_POSITION_FILENAME );
        create_file( prefix, NON_SMOOTHED_SPEED_FILENAME );
    }
}

void BehaviorReader::Process(ProcessingContext& context) {
    
    VideoTrackData* data_in = nullptr;
    std::vector<BehaviorData*> data_out( batch_size_ );
    
    std::vector<std::uint64_t> ts_in( batch_size_ );
    std::vector<double> position_in( batch_size_ );
    std::vector<std::uint64_t> ts_out( batch_size_ );
    std::vector<double> speed_out( batch_size_ );
    
    std::vector<double> position_register( speed_calculator_->total_delay(), 0 );
    
    unsigned int in_maze_to_batch_size = 0;
    
    double linearized_value;
    unsigned int i, j, idx;
    bool ret;
     
    while ( !context.terminated() ) {
        
        // read videotrack data element by element
        if (!data_in_port_->slot(0)->RetrieveData( data_in )) {break;}
        ++ n_received_;

        // linearize 2D position
        switch( linearization_mode_) {
        case ( LinearizationMode::MATRIX ):
            linearized_value = linearization_matrix_[data_in->x()][data_in->y()];
            break;
        case ( LinearizationMode::X ):
            linearized_value = data_in->x();
            break;
        case ( LinearizationMode::Y ):
            linearized_value = data_in->y();
            break;
        default:
            throw ProcessingError("Unexpected linearization mode.", name() );
        }     
        
        // if position is within the environment, use it for computing speed
        // TODO: if not in the environment, interpolate if only few samples 
        // are missing
        if ( linearized_value < 0 ) {
            ++ n_outside_environment_;
            // TODO: use sample to process speed if there are only few out-of-maze points
            LOG(DEBUG) << name() << ". Out-maze point received (X = " << data_in->x()
                << "\tY = " << data_in->y() << " ).";
        } else {
            ++ in_maze_to_batch_size;
            idx = in_maze_to_batch_size - 1;
            position_in[idx] = linearized_value / conversion_factor_;
            ts_in[idx] = data_in->hardware_timestamp();
        }
             
        if ( in_maze_to_batch_size == batch_size_ ) {
            
            // if enough samples have been read, compute speed and adjusted timestamps
            ret = speed_calculator_->compute_speed( ts_in, position_in, ts_out, speed_out ); 
            assert (ret );
            in_maze_to_batch_size = 0;
            
            // save intermediate signals in test mode
            if ( context.test() ) {
                save_intermediate_signals();
            }
            
            // set output
            data_out = data_out_port_->slot(0)->ClaimDataN( batch_size_, false );
            auto speed_sign = speed_calculator_->speed_sign();
            for ( i=0; i<batch_size_; ++i ) {             
                data_out[i]->set_speed( speed_out[i] ); 
                data_out[i]->set_speed_sign( speed_sign[i] );
                data_out[i]->set_hardware_timestamp( ts_out[i] );
                data_out[i]->set_source_timestamp();
            }
            
            // position is also shifted backwards to match the shift of timestamps
            // that takes place inside the speed calculator
            for ( i=0; i<speed_calculator_->total_delay(); ++i ) {
                data_out[i]->set_linear_position( position_register[i] );
            }
            j = 0;
            for ( i=speed_calculator_->total_delay(); i<batch_size_; ++i ) {
                data_out[i]->set_linear_position( position_in[j] );
                ++ j; // j iterates from 0 to (batch_size_ - delay)
            }
            
            // update position_register with last (unprocessed) position values
            // from position_in
            j = 0;
            for ( i=batch_size_-speed_calculator_->total_delay(); i<batch_size_; ++i ) {
                position_register[j] = position_in[i];
                ++ j;
            }
            
            data_out_port_->slot(0)->PublishData();
        }
   
        data_in_port_->slot(0)->ReleaseData();
        
        log_out_of_maze();
    }  
}

void BehaviorReader::Postprocess( ProcessingContext& context ) {
    
    log_out_of_maze();
    LOG(INFO) << name()<< ". Streamed " << data_out_port_->slot(0)->nitems_produced()
        << " data packets";
    
    n_received_ = 0;
    n_outside_environment_ = 0;
}

void BehaviorReader::Unprepare( GlobalContext& context ) {

    if ( linearization_mode_ == LinearizationMode::MATRIX ) {
        for (unsigned int i=0; i < static_cast<unsigned int>(NLX_VIDEO_RESOLUTION[0]); ++i) {
            free( linearization_matrix_[i]); linearization_matrix_[i] = nullptr;
        }
        linearization_matrix_ = nullptr;
    }
}

void BehaviorReader::log_out_of_maze() {
    
    LOG_IF(UPDATE, (n_received_%update_interval_== 0) ) << name() <<
        ": " << n_received_ << " video records received, of which " <<
        n_outside_environment_ << " outside the environment.";
}

void BehaviorReader::save_intermediate_signals() {
    
    streams_[SMOOTH_POSITION_FILENAME]->write(
        reinterpret_cast<const char*>(
            speed_calculator_->smoothed_position().data() ),
            speed_calculator_->smoothed_position().size() * sizeof(double) );

    streams_[NON_SMOOTHED_SPEED_FILENAME]->write(
        reinterpret_cast<const char*>(
            speed_calculator_->unsmooothed_speed().data() ),
            speed_calculator_->unsmooothed_speed().size() * sizeof(double) );
}