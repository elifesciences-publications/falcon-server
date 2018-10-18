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

#include "videotrackreader.hpp"

#include <chrono>

void VideoTrackReader::Configure(const YAML::Node& node, const GlobalContext& context) {
    
    ip_address_ = node["ip_address"].as<decltype(ip_address_)>( );
    vt_id_ = node["vt_id"].as<decltype(vt_id_)>( DEFAULT_VT_ID );
    n_max_consecutive_occlusions_ = node["n_max_consecutive_occlusions"].as
        <decltype(n_max_consecutive_occlusions_)>( DEFAULT_N_MAX_CONSECUTIVE_OCCLUSIONS );
    
    no_videotrack_coordinates_ = DEFAUL_NO_VIDEOTRACK;
    
    // set frequency of updates
    auto value = node["update_interval"].as<decltype(
        update_interval_)>( DEFAULT_UPDATE_INTERVAL_SEC );
    if ( value==0 ) {
        update_interval_ = std::numeric_limits<decltype(update_interval_)>::max();
    } else {
        update_interval_ = value * NLX_VIDEO_SAMPLING_FREQUENCY;
    }
}

void VideoTrackReader::CreatePorts() {

    data_out_port_ = create_output_port(
        VIDEOTRACKDATA_S,
        VideoTrackDataType( NLX_VIDEO_SAMPLING_FREQUENCY, NLX_VIDEO_RESOLUTION ),
        PortOutPolicy( SlotRange(1) ) );
}

void VideoTrackReader::CompleteStreamInfo( ) {
    
    data_out_port_->streaminfo(0).datatype().Finalize();
    data_out_port_->streaminfo(0).Finalize(
        data_out_port_->streaminfo(0).datatype().sample_rate() );
}

void VideoTrackReader::Prepare( GlobalContext& context ) {

    socket_.reset( new zmq::socket_t( context.zmq(), ZMQ_PAIR) );
}

void VideoTrackReader::Preprocess( ProcessingContext& context ) {
    
    error_code_ = ErrorNLXVT::Code::UNKNOWN;
    out_of_order_ = false;
    
    socket_->connect( ip_address_.c_str() );
}

void VideoTrackReader::Process(ProcessingContext& context) {
    
    zmq::message_t buffer;
    VideoTrackData* data_out = nullptr; 
    VideoRec* vt_record = nullptr;
    unsigned int n_consecutive_no_videotrack = 0;
    bool no_videotrack = false, tracking_mode_on = true;
    bool no_tracking_mode_message_sent = false;
    bool previous_no_videotrack = false;
    auto last_timestamp = NO_TIMESTAMP;
    
    while ( !context.terminated() ) {

        if ( socket_->recv( &buffer, ZMQ_DONTWAIT ) > 0) {
        
            vt_record = static_cast<VideoRec*>( buffer.data() );

            if ( check_validity( vt_record, vt_id_ ) ) {

                ++ packet_counter_;

                data_out = data_out_port_->slot(0)->ClaimData( false );

                // check for out-of-order timestamps
                // this should never happen as we are dealing with a TCP stream
                if ( last_timestamp != NO_TIMESTAMP and
                vt_record->qwTimeStamp <= last_timestamp ) {
                    out_of_order_ = true;
                    LOG(ERROR) << name() <<
                        ". Received VT record with out-of-order timestamp.";
                }
                last_timestamp = vt_record->qwTimeStamp;

                // detect occlusion
                no_videotrack =
                    vt_record->dnextracted_x == no_videotrack_coordinates_[0] and
                    vt_record->dnextracted_y == no_videotrack_coordinates_[1];
                // TODO: occlusions that do not result in a known vt point can be detected
                // with a dedicated method (e.g. 2d distance between consecutive vt points) 
                
                // check if this is an occlusion
                if ( no_videotrack and
                n_consecutive_no_videotrack < n_max_consecutive_occlusions_ ) {
                        
                    ++ n_occlusions_;
                    data_out->mark_as_occluded();
                    
                    data_out->set_x( last_nonoccluded_measures_[0] );
                    data_out->set_y( last_nonoccluded_measures_[1] );
                    data_out->set_angle( last_nonoccluded_measures_[2] ); 
                    
                    if ( previous_no_videotrack ) {
                        ++ n_consecutive_no_videotrack;
                    }                    
                    
                    // even if there is an occlusion we are still tracking something,
                    // therefore tracking mode should be switched on
                    if ( not tracking_mode_on ) {
                        tracking_mode_on = true;
                        LOG(UPDATE) << name() << ". An object is now being tracked.";
                        no_tracking_mode_message_sent = false;
                    }
                    
                } else {
                    // no occlusion because we either have a target at (x,y) != (0,0)
                    // or because there is no target
                    
                    data_out->set_x( vt_record->dnextracted_x );
                    data_out->set_y( vt_record->dnextracted_y );
                    data_out->set_angle( vt_record->dnextracted_angle );
                    
                    last_nonoccluded_measures_ = {
                    vt_record->dnextracted_x ,
                    vt_record->dnextracted_y,
                    vt_record->dnextracted_angle };
                    
                    if ( not no_videotrack ) {
                        n_consecutive_no_videotrack = 0;
                    }
                    
                    // turn on tracking mode if we are tracking
                    if ( not tracking_mode_on and not no_videotrack ) {
                        tracking_mode_on = true;
                        LOG(UPDATE) << name() << ". An object is now being tracked.";
                        no_tracking_mode_message_sent = false;
                    }
                    
                }

                data_out->set_hardware_timestamp( vt_record->qwTimeStamp );
                data_out->set_source_timestamp();

                data_out_port_->slot(0)->PublishData();
            }
            
            previous_no_videotrack = no_videotrack;
            
            if ( n_consecutive_no_videotrack >= n_max_consecutive_occlusions_ and
            not no_tracking_mode_message_sent ) {
                LOG( UPDATE ) << name() << ". No trackable object present in the room,"
                    << " streaming null videotrack coordinates (" << no_videotrack_coordinates_[0]
                    << "," << no_videotrack_coordinates_[1] << ").";
                tracking_mode_on = false;
                no_tracking_mode_message_sent = true;
            }

            if ( packet_counter_==1 ) {
                first_packet_arrival_time_ = Clock::now();
                LOG(UPDATE) << name() << ": Received first video record.";
            }

            LOG_IF(UPDATE, (packet_counter_%update_interval_== 0) ) << name() <<
                ": " << packet_counter_ << " video records received ( "
                << packet_counter_ / NLX_VIDEO_SAMPLING_FREQUENCY << " s)." <<
                " # occlusions = " << n_occlusions_;
        }  
    }
}

void VideoTrackReader::Postprocess( ProcessingContext& context ) {
    
    LOG(INFO) << name() << ". Streamed " << data_out_port_->slot(0)->nitems_produced()
        << " data packets";
    
    LOG(INFO) << name() << ". There were " << n_occlusions_ << " occlusions.";
    n_occlusions_ = 0;
    
    std::chrono::milliseconds runtime( 
        std::chrono::duration_cast<std::chrono::milliseconds>(
        Clock::now() - first_packet_arrival_time_) );
    
    LOG(INFO) << name() << ". Finished reading : "
        << packet_counter_ << " packets received over "
        << static_cast<double>(runtime.count()/1000) << " seconds at a rate of " 
        << packet_counter_ / static_cast<double>(runtime.count()/1000) << " packets/second."; 
    packet_counter_ = 0;
    
    LOG_IF(ERROR, out_of_order_ ) << name() <<
        ". At least one out-of-order VT packet was received";
    
    socket_->close();
}

bool VideoTrackReader::check_validity( VideoRec* vt_record, std::uint16_t vt_id ) {
    
    is_valid_ = valid_nlx_vt( vt_record, vt_id, error_code_,
        data_out_port_->slot(0)->datatype().resolution() );
    
    if ( not is_valid_ ) {
        switch ( error_code_ ) {
        case( ErrorNLXVT::Code::SWSTX ):
            LOG(ERROR) << name() << ". Invalid VideoTrack SWSTX.";
            break;
        case( ErrorNLXVT::Code::SWID ):
            LOG(ERROR) << name() << ". Invalid VideoTrack ID.";
            break;
        case( ErrorNLXVT::Code::SWDATA_SIZE ):
            LOG(ERROR) << name() << ". Invalid VideoTrack SW data size.";
            break;
        case( ErrorNLXVT::Code::NEGATIVE_COORDINATE ):
            LOG(ERROR) << name() <<
                ". Invalid VideoTrack Data: unexpected negative coordinate.";
            break;
        case( ErrorNLXVT::Code::OUT_OF_RESOLUTION ):
            LOG(ERROR) << name() <<
                ". Invalid VideoTrack Data: unexpected out-of-resolution value.";
            break;
        default:
            LOG(ERROR) << name() << ". Invalid VideoTrack Data:  unknown error code.";
            break;
        }
    }   
    return is_valid_;
}

