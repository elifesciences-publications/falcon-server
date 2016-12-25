#include "burstdetector.hpp"
#include "g3log/src/g2log.hpp"

void BurstDetector::Configure(const YAML::Node& node, const GlobalContext& context) {
    
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
}

void BurstDetector::CreatePorts() {
    
    data_in_port_ = create_input_port(
        "mua",
        MUADataType( ),
        PortInPolicy( SlotRange(1) ) );
    
    data_out_port_ = create_output_port(
        "events",
        EventDataType( "burst" ),
        PortOutPolicy( SlotRange(1) ) );

    stats_out_port_ = create_output_port(
        "statistics",
        MultiChannelDataType<double>( ChannelRange(N_STATS_OUT) ),
        PortOutPolicy( SlotRange(1) ) );
    
    threshold_ = create_writable_shared_state(
        "threshold_uV2",
        0.0,
        Permission::READ,
        Permission::READ );
    
    signal_mean_ = create_writable_shared_state(
        "mean_uV2",
        0.0,
        Permission::READ,
        Permission::READ );
    
    signal_dev_ = create_writable_shared_state(
        "deviation_uV2",
        0.0,
        Permission::READ,
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
    
    burst_ = create_writable_shared_state(
        "burst",
        false,
        Permission::READ,
        Permission::READ );
    
    bin_size_mua_ = create_readable_shared_state(
        "bin_size",
        1.0,
        Permission::WRITE,
        Permission::READ );
}

void BurstDetector::CompleteStreamInfo( ) {
    
    stats_nsamples_ = stats_buffer_size_ * 1e3 / data_in_port_->streaminfo(0).datatype().bin_size();
    if ( stats_nsamples_ == 0 ) {
        throw ProcessingCreatePortsError("Stats buffersize is smaller than MUA bin size.", name());
    }
    
    stats_out_port_->streaminfo(0).datatype().Finalize(
        stats_nsamples_, N_STATS_OUT,
        1 / data_in_port_->streaminfo(0).datatype().bin_size() * 1e3 );
    stats_out_port_->streaminfo(0).Finalize(
        data_in_port_->streaminfo(0).stream_rate() );
    
    data_out_port_->streaminfo(0).datatype().Finalize();
    data_out_port_->streaminfo(0).Finalize( IRREGULARSTREAM );
}

void BurstDetector::Preprocess( ProcessingContext& context ) {
    
    signal_mean_->set(0);
    signal_dev_->set(0);
    threshold_->set(0);
    block_ = 0;
        
    sample_rate_ = 1 / data_in_port_->slot(0)->streaminfo().datatype().bin_size() * 1e3;
    
    LOG(UPDATE) << name() << ". Incoming Sample rate: " << sample_rate_;
    
    burn_in_ = initial_smooth_time_ * sample_rate_;
    
    if ( burn_in_ == 0 ) {
        burn_in_ = 1;
        LOG(UPDATE) << name() << ". " << SMOOTH_TIME_S <<
            " too low. Burn-in set to 1 sample.";
    }

    double alpha = 1.0/burn_in_;
    
    running_statistics_.reset( new dsp::algorithms::RunningMeanMAD(
        alpha, burn_in_, false ) );
    threshold_detector_.reset( new dsp::algorithms::ThresholdCrosser( 0 ) );
}

void BurstDetector::Process(ProcessingContext& context) {
    
    MUAData* data_in = nullptr;
    EventData* event_out = nullptr;
    MultiChannelData<double>* stats_out = nullptr;
    
    double value, test_value;
    auto stats_nsamples_counter = stats_nsamples_;
    auto burnin_update_sent = false;
    
    // burn-in period
    while (running_statistics_->is_burning_in() && !context.terminated()) {
        
        if (!data_in_port_->slot(0)->RetrieveData( data_in )) {break;}
        
        if ( not burnin_update_sent ) {
            LOG(UPDATE) << name() << ": burn-in period starting (" <<
                initial_smooth_time_ << " seconds)";
            burnin_update_sent = true;
        }
        
        running_statistics_->add_sample( data_in->mua() );      
        data_in_port_->slot(0)->ReleaseData(); 
    }
    
    if ( not running_statistics_->is_burning_in() ) {
        LOG(UPDATE) << name() << ": end of burn-in period";
        LOG(UPDATE) << name() << ": statistics: center = " <<
            running_statistics_->center()
            << ", dispersion = " << running_statistics_->dispersion();

        LOG(UPDATE) << name() << ": burst detection starts now with initial threshold of "
            << (threshold_dev_->get() * running_statistics_->dispersion());
    }
    
    while ( !context.terminated() ) {
        
        // retrieve new data
        if (!data_in_port_->slot(0)->RetrieveData( data_in )) {break;}
        
        // update threshold and alpha only once for an incoming data bucket
        threshold_->set( threshold_dev_->get() * running_statistics_->dispersion() );
        threshold_detector_->set_threshold( threshold_->get() ); 
        running_statistics_->set_alpha( 1.0/(smooth_time_->get() * sample_rate_) );
            
        value = data_in->mua();
        test_value = std::abs( value - running_statistics_->center() );

        if ( stats_out_->get() ) {

            if ( stats_nsamples_counter==stats_nsamples_ ) {
                stats_out_port_->slot(0)->PublishData();
                stats_out = stats_out_port_->slot(0)->ClaimData(false);
                stats_out->set_source_timestamp( );
                stats_out->set_hardware_timestamp( data_in->hardware_timestamp() );
                stats_nsamples_counter = 0;
            }
            
            stats_out->set_data_sample( stats_nsamples_counter, 0,
                test_value );
            stats_out->set_data_sample( stats_nsamples_counter, 1,
                threshold_detector_->threshold() );
            stats_out->set_sample_timestamp( stats_nsamples_counter,
                data_in->hardware_timestamp() );

            ++stats_nsamples_counter;
        }

        if ( block_ > 0 ) {
            --block_;
            continue;
        } else if ( burst_->get() ) {
            burst_->set( false );
        }

        if ( threshold_detector_->has_crossed_up( test_value )) {
            block_ = static_cast<decltype(block_)>
                ( detection_lockout_time_->get() * sample_rate_ / 1e3 );
            if ( stream_events_->get() ) {
                event_out = data_out_port_->slot(0)->ClaimData(false);
                event_out->set_source_timestamp( data_in->source_timestamp() );
                event_out->set_hardware_timestamp( data_in->hardware_timestamp() );
                data_out_port_->slot(0)->PublishData();
            }
        }

        running_statistics_->add_sample( value );
        
        data_in_port_->slot(0)->ReleaseData();
        
        signal_mean_->set( running_statistics_->center() );
        signal_dev_->set( running_statistics_->dispersion() );
    }  
}

void BurstDetector::Postprocess( ProcessingContext& context ) {
    
    LOG(INFO) << name()<< ". Streamed " << data_out_port_->slot(0)->nitems_produced()
        << " burst events.";
}
