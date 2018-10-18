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

#include "videotrackdatafilestreamer.hpp"

#include "utilities/string.hpp"

void VideoTrackDataFileStreamer::Configure(const YAML::Node& node, const GlobalContext& context) {
    
    path_to_x_ = node["path_to_x"].as<std::string>( NULL_PATH );
    path_to_y_ = node["path_to_y"].as<std::string>( NULL_PATH );
    path_to_angle_ = node["path_to_angle"].as<std::string>( NULL_PATH );
    path_to_occlusions_ = node["path_to_occlusions"].as<std::string>( NULL_PATH);
    
    load_x_ = (path_to_x_ != NULL_PATH );
    load_y_ = (path_to_y_ != NULL_PATH );
    load_angle_ = (path_to_angle_ != NULL_PATH );
    load_occlusions_ = (path_to_occlusions_ != NULL_PATH );
    
    // check paths
    if ( ( not load_x_ ) and ( not load_y_ ) and ( not load_y_ ) ) {
        auto err_msg =
            "No paths to NPY files were entered. At least one path must be specified.";
        throw ProcessingConfigureError( err_msg, name() );
    }
    if ( load_x_ ) {
        if ( not path_exists( path_to_x_ ) ) {
            auto err_msg = "Path to x-coordinate values does not exist (" + path_to_x_ + ")";
            throw ProcessingConfigureError( err_msg, name() );
        }
        LOG( INFO ) << name() << ". X-coordinates will be loaded from file.";
    }
    if ( load_y_ ) {
        if ( not path_exists( path_to_y_ ) ) {
            auto err_msg = "Path to y-coordinate values does not exist (" + path_to_y_ + ")";
            throw ProcessingConfigureError( err_msg, name() );
        }
        LOG( INFO ) << name() << ". Y-coordinates will be loaded from file.";
    }
    if ( load_angle_ ) {
        if ( not path_exists( path_to_angle_ ) ) {
            auto err_msg = "Path to angle values values does not exist (" + path_to_angle_ + ")";
            throw ProcessingConfigureError( err_msg, name() );
        }
        LOG( INFO ) << name() << ". Angle values will be loaded from file.";
    }
    if ( load_occlusions_ ) {
        if ( not path_exists( path_to_occlusions_ ) ) {
            auto err_msg = "Path to angle values values does not exist (" +
                path_to_occlusions_ + ")";
            throw ProcessingConfigureError( err_msg, name() );
        }
        LOG( INFO ) << name() << ". Occlusion mask will be loaded from file.";
    }
    
    sample_rate_ = node["sample_rate"].as<decltype(sample_rate_)>(
        DEFAULT_SAMPLE_RATE );
    streaming_rate_ = node["streaming_rate"].as<decltype(streaming_rate_)>(
        DEFAULT_STREAMING_RATE );
    initial_timestamp_ = node["initial_timestamp"].as<double>( DEFAULT_INITIAL_TS );
    
    // workaround to read std::array from yaml 
    std::vector<int> default_resolution =
        { NLX_VIDEO_RESOLUTION[0], NLX_VIDEO_RESOLUTION[1] };
    auto resolution =
        node["resolution"].as<decltype(default_resolution)>( default_resolution );
    assert( resolution.size() == 2 );
    resolution_[0] = resolution[0]; resolution_[1] = resolution[1];
}

void VideoTrackDataFileStreamer::CreatePorts() {

    data_out_port_ = create_output_port(
        VIDEOTRACKDATA_S,
        VideoTrackDataType( sample_rate_, resolution_ ),
        PortOutPolicy( SlotRange(1) ) );
}

void VideoTrackDataFileStreamer::CompleteStreamInfo( ) {
    
    data_out_port_->streaminfo(0).datatype().Finalize();
    data_out_port_->streaminfo(0).Finalize( streaming_rate_ );
}

void VideoTrackDataFileStreamer::Prepare( GlobalContext& context) {
    
    // initialize with different values
    auto n_x_values = std::numeric_limits<uint32_t>::max();
    auto n_y_values = std::numeric_limits<uint32_t>::max() - 1; 
    auto n_angles = std::numeric_limits<uint32_t>::max() - 2;
    auto size_occlusion_mask = std::numeric_limits<uint32_t>::max() - 3;
    
    if ( load_x_ ) {
        
        FILE* fp_x = nullptr;
        
        if ( ( fp_x = fopen( path_to_x_.c_str(), "r") ) == NULL ) {
            auto err_msg = "Cannot open the NPY file. Check the file path "
                + path_to_x_;
            LOG(ERROR) << ". " << err_msg;
            throw ProcessingPrepareError( err_msg, name() );
        }
        
        if ( get_1D_array_len( fp_x, &n_x_values ) != 0 ) {
            auto err_msg = "Cannot read the length of the NPY file.";
            LOG(ERROR) << ". " << err_msg;
            throw ProcessingPrepareError( err_msg, name() );
        }
        
        n_packets_to_stream_ = n_x_values;
        
        if ( ( loaded_x_values_ = get_1D_array_int32( fp_x )) == NULL ) {
            auto err_msg = "Cannot read positions from NPY file.";
            LOG(ERROR) << ". " << err_msg;
            throw ProcessingPrepareError(err_msg, name());
        }
        LOG( INFO ) << name() << ". " << n_x_values <<
            " X-coordinates are loaded from file and are ready to be streamed.";
        
        fclose( fp_x );
    }
    
    if ( load_y_ ) {
        
        FILE* fp_y = nullptr;
        
        if ( ( fp_y = fopen( path_to_y_.c_str(), "r") ) == NULL ) {
            auto err_msg = "Cannot open the NPY file. Check the file path "
                + path_to_x_;
            LOG(ERROR) << ". " << err_msg;
            throw ProcessingPrepareError( err_msg, name() );
        }
        
        if ( get_1D_array_len( fp_y, &n_y_values ) != 0 ) {
            auto err_msg = "Cannot read the length of the NPY file.";
            LOG(ERROR) << ". " << err_msg;
            throw ProcessingPrepareError( err_msg, name() );
        }
        
        if ( load_x_ ) {
            if ( n_y_values != n_x_values ) {
                auto err_msg = "Different number of X and Y coordinates.";
                LOG(ERROR) << ". " << err_msg;
                throw ProcessingPrepareError( err_msg, name() );
            }
        }
        
        n_packets_to_stream_ = n_y_values;
        
        if ( ( loaded_y_values_ = get_1D_array_int32( fp_y )) == NULL ) {
            auto err_msg = "Cannot read positions from NPY file.";
            LOG(ERROR) << ". " << err_msg;
            throw ProcessingPrepareError(err_msg, name());
        }
        LOG( INFO ) << name() << ". " << n_y_values <<
            " Y-coordinates are loaded from file and are ready to be streamed.";
        
        fclose( fp_y );
    }
    
    if ( load_angle_ ) {
        
        FILE* fp_angle = nullptr;
        
        if ( ( fp_angle = fopen( path_to_angle_.c_str(), "r") ) == NULL ) {
            auto err_msg = "Cannot open the NPY file. Check the file path "
                + path_to_x_;
            LOG(ERROR) << ". " << err_msg;
            throw ProcessingPrepareError( err_msg, name() );
        }
        
        if ( get_1D_array_len( fp_angle, &n_angles ) != 0 ) {
            auto err_msg = "Cannot read the length of the NPY file.";
            LOG(ERROR) << ". " << err_msg;
            throw ProcessingPrepareError( err_msg, name() );
        }
        
        if ( load_x_ or load_y_ ) {
            if ( n_angles != n_x_values and n_angles != n_y_values ) {
                auto err_msg = "Different number of angle values and XY coordinates.";
                LOG(ERROR) << ". " << err_msg;
                throw ProcessingPrepareError( err_msg, name() );
            }
        }
        
        n_packets_to_stream_ = n_angles;
        
        if ( ( loaded_angle_values_ = get_1D_array_int32( fp_angle )) == NULL ) {
            auto err_msg = "Cannot read positions from NPY file.";
            LOG(ERROR) << ". " << err_msg;
            throw ProcessingPrepareError(err_msg, name());
        }
        LOG( INFO ) << name() << ". " << n_angles <<
            " angle values are loaded from file and are ready to be streamed.";
    }
    
    if ( load_occlusions_ ) {
        
        FILE* fp_occlusions = nullptr;
        
        if ( ( fp_occlusions = fopen( path_to_occlusions_.c_str(), "r") ) == NULL ) {
            auto err_msg = "Cannot open the NPY file. Check the file path "
                + path_to_occlusions_;
            LOG(ERROR) << ". " << err_msg;
            throw ProcessingPrepareError( err_msg, name() );
        }
        
        if ( get_1D_array_len( fp_occlusions, &size_occlusion_mask ) != 0 ) {
            auto err_msg = "Cannot read the length of the NPY file.";
            LOG(ERROR) << ". " << err_msg;
            throw ProcessingPrepareError( err_msg, name() );
        }
        
        if ( n_packets_to_stream_!= n_x_values ) {
            auto err_msg =
            "Occlusion mask size does not match with the number of packets to be streamed";
            LOG(ERROR) << ". " << err_msg;
            throw ProcessingPrepareError( err_msg, name() );
        }
        
        if ( ( loaded_y_values_ = get_1D_array_int32( fp_occlusions )) == NULL ) {
            auto err_msg = "Cannot read positions from NPY file.";
            LOG(ERROR) << ". " << err_msg;
            throw ProcessingPrepareError(err_msg, name());
        }
        LOG( INFO ) << name() << ". " << n_y_values <<
            " Y-coordinates are loaded from file and are ready to be streamed.";
        
        fclose( fp_occlusions );
    }
    
    // generate hardware timestamps
    double last_time_us =   initial_timestamp_ + 
                            (n_packets_to_stream_ - 1) * 1e6 / sample_rate_ ;
    std::vector<double> hw_timestamps =
        linspace(   static_cast<double>(initial_timestamp_), last_time_us,
                    n_packets_to_stream_);
    for (auto& el: hw_timestamps) {
        generated_hw_timestamps_.push_back( static_cast<uint64_t>( std::round(el) ) );
    }
    LOG(DEBUG) << name() << ". First generated TS: " << generated_hw_timestamps_[0]
        << ". Last generated TS: " << generated_hw_timestamps_[n_packets_to_stream_-1];
}

void VideoTrackDataFileStreamer::Preprocess( ProcessingContext& context ) {
    
    //
}

void VideoTrackDataFileStreamer::Process( ProcessingContext& context ) {
    
    VideoTrackData* data_out = nullptr;
    auto delay = std::chrono::microseconds( static_cast<unsigned int>(
        1e6 / streaming_rate_ ) );  
    decltype(n_packets_to_stream_) i = 0;

    while ( !context.terminated() and 
    data_out_port_->slot(0)->nitems_produced() < n_packets_to_stream_ ) {
        
        data_out = data_out_port_->slot(0)->ClaimData( true );
        
        if ( load_x_ ) {
            data_out->set_x( loaded_x_values_[i] );
        }
        if ( load_y_ ) {
            data_out->set_y( loaded_y_values_[i] );
        }
        if ( load_angle_ ) {
            data_out->set_angle( loaded_angle_values_[i] );
        }
        if ( load_occlusions_ ) {
            if ( loaded_occlusions_[i] ) {
                data_out->mark_as_occluded();
            }
        }

        data_out->set_hardware_timestamp( generated_hw_timestamps_[i] );
        data_out->set_source_timestamp();
        
        ++ i;
        
        data_out_port_->slot(0)->PublishData();
        
        std::this_thread::sleep_for( delay );
    }  
}

void VideoTrackDataFileStreamer::Postprocess( ProcessingContext& context ) {
    
    LOG(INFO) << name()<< ". Streamed all " << data_out_port_->slot(0)->nitems_produced()
        << " data packets. PRESS 's' TO STOP THE GRAPH. ";
    n_packets_to_stream_ = 0;
}

void VideoTrackDataFileStreamer::Unprepare( GlobalContext& context ) {

    if ( load_x_ ) {
        free( loaded_x_values_ ); loaded_x_values_ = nullptr;
    }
    if ( load_y_ ) {
        free( loaded_y_values_ ); loaded_y_values_ = nullptr;
    }
    if ( load_x_ ) {
        free( loaded_angle_values_ ); loaded_angle_values_ = nullptr;
    }
    if ( load_occlusions_) {
        free( loaded_occlusions_ ); loaded_occlusions_ = nullptr;
    }
}

