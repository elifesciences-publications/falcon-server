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

#include "rippledetector.hpp"

#include "g3log/src/g2log.hpp"

void RippleDetector::Configure( const YAML::Node & node, const GlobalContext& context ) {
    
    initial_threshold_dev_ = node[THRESHOLD_DEV_S].as<decltype(initial_threshold_dev_)>
        ( DEFAULT_THRESHOLD_DEV );
    
    initial_smooth_time_ = node[SMOOTH_TIME_S].as<decltype(initial_smooth_time_)>(
        DEFAULT_SMOOTH_TIME );
    if ( initial_smooth_time_ <= 0 ) {
        auto err_msg = SMOOTH_TIME_S + " must be a positive number.";
        throw ProcessingConfigureError( err_msg, name() );
    }
    
    initial_detection_lockout_time_ =
        node[DETECTION_LOCKOUT_TIME_S].as<decltype( initial_detection_lockout_time_ )>
            ( DEFAULT_DETECTION_LOCKOUT_TIME );
    if ( initial_detection_lockout_time_ == 0 ) {
        throw ProcessingConfigureError(
            "Minimum detection lock out time must greater than 0 ms.", name() );
    }
    
    default_stream_events_ = node[STREAM_EVENTS_S].as<decltype(default_stream_events_)>
        ( DEFAULT_STREAM_EVENTS );
    
    initial_stats_out_ = node[STREAM_STATISTICS_S].as<decltype(initial_stats_out_)>
        ( DEFAULT_STREAM_STATISTICS );
    
    stats_buffer_size_ =
        node[STATISTICS_BUFFER_SIZE_S].as<decltype(stats_buffer_size_)>
            ( DEFAULT_STATISTICS_BUFFER_SIZE );
    if ( stats_buffer_size_<=0 ) {
        throw ProcessingConfigureError("Buffer size should be equal larger than zero.",
            name());
    }
    
    stats_downsample_factor_ =
        node[STATISTICS_DOWNSAMPLE_FACTOR_S].as<decltype(stats_downsample_factor_)>(
            DEFAULT_STATISTICS_DOWNSAMPLE_FACTOR );
    if ( stats_downsample_factor_<=0 ) {
        throw ProcessingConfigureError(
            "Downsample factor should larger than zero.", name() );
    }
    
    use_power_ = node["use_power"].as<decltype(use_power_)>( true );
}

void RippleDetector::CreatePorts( ) {
    
    data_in_port_ = create_input_port(
        "data",
        MultiChannelDataType<double>( ChannelRange(1,256) ),
        PortInPolicy( SlotRange(1) ) );
    
    event_out_port_ = create_output_port(
        "events",
        EventDataType( "ripple" ),
        PortOutPolicy( SlotRange(1) ) );
    
    stats_out_port_ = create_output_port(
        "statistics",
        MultiChannelDataType<double>( ChannelRange(N_STATS_OUT) ),
        PortOutPolicy( SlotRange(1) ) );
    
    threshold_ = create_writable_shared_state(
        "threshold",
        0.0,
        Permission::NONE,
        Permission::READ );
    
    signal_mean_ = create_writable_shared_state(
        "mean",
        0.0,
        Permission::NONE,
        Permission::READ );
    
    signal_dev_ = create_writable_shared_state(
        "deviation",
        0.0,
        Permission::NONE,
        Permission::READ );
    
    threshold_dev_ = create_readable_shared_state(
        THRESHOLD_DEV_S,
        initial_threshold_dev_,
        Permission::READ,
        Permission::WRITE );
    
    detection_lockout_time_ = create_readable_shared_state(
        DETECTION_LOCKOUT_TIME_S,
        initial_detection_lockout_time_,
        Permission::READ,
        Permission::WRITE );
    
    stream_events_ = create_readable_shared_state(
        STREAM_EVENTS_S,
        default_stream_events_,
        Permission::READ,
        Permission::WRITE );
    
    smooth_time_ = create_readable_shared_state(
        SMOOTH_TIME_S,
        initial_smooth_time_,
        Permission::READ,
        Permission::WRITE );
    
    stats_out_ = create_readable_shared_state(
        STREAM_STATISTICS_S,
        initial_stats_out_,
        Permission::READ,
        Permission::WRITE );
        
    ripple_ = create_writable_shared_state(
        "ripple",
        false,
        Permission::READ,
        Permission::READ );
}

void RippleDetector::CompleteStreamInfo( ) {
    
    stats_nsamples_ = stats_buffer_size_ *
        data_in_port_->streaminfo(0).datatype().sample_rate() / stats_downsample_factor_;
    stats_nsamples_ = std::max( stats_nsamples_, 1UL );
    
    stats_out_port_->streaminfo(0).datatype().Finalize(
        stats_nsamples_, N_STATS_OUT,
        data_in_port_->streaminfo(0).datatype().sample_rate() / stats_downsample_factor_ );
    stats_out_port_->streaminfo(0).Finalize(
        data_in_port_->streaminfo(0).stream_rate() );
    
    event_out_port_->streaminfo(0).datatype().Finalize();
    event_out_port_->streaminfo(0).Finalize( );
    
}

void RippleDetector::Preprocess( ProcessingContext& context ) {
    
    signal_mean_->set(0);
    signal_dev_->set(0);
    threshold_->set(0);
    block_ = 0;
        
    sample_rate_ = data_in_port_->slot(0)->streaminfo().datatype().sample_rate();
    burn_in_ = initial_smooth_time_ * sample_rate_;

    double alpha = 1.0/burn_in_;
    
    running_statistics_.reset(
        new dsp::algorithms::RunningMeanMAD( alpha, burn_in_, false ) );
    threshold_detector_.reset( new dsp::algorithms::ThresholdCrosser( 0 ) );
}

void RippleDetector::Process( ProcessingContext& context ) {
    
    MultiChannelData<double>* data_in = nullptr;
    EventData* event_out = nullptr;
    MultiChannelData<double>* stats_out = nullptr;
    
    double value, test_value;
    unsigned int s;
    auto stats_nsamples_counter = stats_nsamples_;
    decltype(stats_downsample_factor_) stats_skip_counter = 0;
    auto burnin_update_sent = false;
    
    // burn-in period
    while (running_statistics_->is_burning_in() && !context.terminated()) {
        
        if (!data_in_port_->slot(0)->RetrieveData( data_in )) {break;}
        
        if ( not burnin_update_sent ) {
            LOG(UPDATE) << name() << ": burn-in period starting (" <<
                initial_smooth_time_ << " seconds)";
            burnin_update_sent = true;
        }
        for ( unsigned int s=0; s<data_in->nsamples(); ++s ) {
            running_statistics_->add_sample( compute_value( data_in, s ) );
        }
        
        data_in_port_->slot(0)->ReleaseData();
        
    }
    
    if ( not running_statistics_->is_burning_in() ) {
        LOG(UPDATE) << name() << ": end of burn-in period";
        LOG(UPDATE) << name() << ": statistics: center = " <<
            running_statistics_->center()
            << ", dispersion = " << running_statistics_->dispersion();

        LOG(UPDATE) << name() << ": ripple detection starts now with initial threshold of "
            << (threshold_dev_->get() * running_statistics_->dispersion());
    }
    
    // ripple detection
    while ( !context.terminated() ) {
        
        // retrieve new data
        if (!data_in_port_->slot(0)->RetrieveData( data_in )) {break;}
        
        // update threshold and alpha only once for an incoming data bucket
        threshold_->set( threshold_dev_->get() * running_statistics_->dispersion() );
        threshold_detector_->set_threshold( threshold_->get() ); 
        running_statistics_->set_alpha( 1.0/(smooth_time_->get() * sample_rate_) );
        
        // loop through each sample
        for ( s=0; s<data_in->nsamples(); ++s ) {
            
            value = compute_value( data_in, s );
            test_value = std::abs( value - running_statistics_->center() );
            
            if ( stats_out_->get() ) {
                
                if ( stats_nsamples_counter==stats_nsamples_ ) {
                    stats_out_port_->slot(0)->PublishData();
                    stats_out = stats_out_port_->slot(0)->ClaimData(false);
                    stats_out->set_source_timestamp( data_in->source_timestamp() );
                    stats_out->set_hardware_timestamp( data_in->sample_timestamp( s ) );
                    stats_nsamples_counter = 0;
                }
                
                if ( stats_skip_counter==0 ) {
                    
                    stats_out->set_data_sample( stats_nsamples_counter, 0,
                        test_value );
                    stats_out->set_data_sample( stats_nsamples_counter, 1,
                        threshold_detector_->threshold() );
                    stats_out->set_sample_timestamp( stats_nsamples_counter,
                        data_in->sample_timestamp(s) );
                    
                    stats_skip_counter = stats_downsample_factor_;
                    ++stats_nsamples_counter;
                }
                
                --stats_skip_counter;
            }
            
            if ( block_ > 0 ) {
                --block_;
                continue;
            } else if ( ripple_->get() ) {
                ripple_->set( false );
            }
            
            if ( threshold_detector_->has_crossed_up( test_value )) {
                block_ = static_cast<decltype(block_)>
                    ( detection_lockout_time_->get() * sample_rate_ / 1e3 );
                if ( stream_events_->get() ) {
                    event_out = event_out_port_->slot(0)->ClaimData(false);
                    event_out->set_source_timestamp( data_in->source_timestamp() );
                    event_out->set_hardware_timestamp( data_in->sample_timestamp( s ) );
                    event_out_port_->slot(0)->PublishData();
                }
            }
            
            running_statistics_->add_sample( value );
            
        }
        
        data_in_port_->slot(0)->ReleaseData();
        
        signal_mean_->set( running_statistics_->center() );
        signal_dev_->set( running_statistics_->dispersion() );
    }
    
}

void RippleDetector::Postprocess( ProcessingContext& context ) {
    
    LOG(INFO) << name()<< ". Streamed " << event_out_port_->slot(0)->nitems_produced()
        << " ripple events.";
}

inline double RippleDetector::compute_value( MultiChannelData<double>* data_in,
    unsigned int sample ) {
    
    if ( use_power_ ) {
        acc_ =  std::pow( *data_in->begin_sample(sample), 2 );
        for ( auto c=data_in->begin_sample(sample)+1; c!=data_in->end_sample(sample); ++c ) {
            acc_ += std::pow( *c, 2 );
        }

        return acc_/data_in->nchannels();
    }
    
    return data_in->mean_sample(sample);
}
