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

#include "spikedetector.hpp"
#include "g3log/src/g2log.hpp"

void SpikeDetector::Configure( const YAML::Node & node, const GlobalContext& context ) {
    
    initial_threshold_ = node["threshold"].as<decltype(initial_threshold_)>(
        DEFAULT_THRESHOLD );
    invert_signal_ = node["invert_signal"].as<decltype(invert_signal_)>(
        DEFAULT_INVERT_SIGNAL );
    buffer_size_ms_ = node["buffer_size"].as<decltype(buffer_size_ms_)>(
        DEFAULT_BUFFER_SIZE_MS );
    strict_time_bin_check_ =
        node["strict_time_bin_check"].as<decltype(strict_time_bin_check_)>(
        DEFAULT_STRICT_TIME_BIN_CHECK );
    initial_peak_lifetime_ = node["peak_lifetime"].as<decltype(initial_peak_lifetime_)>(
        DEFAULT_PEAK_LIFETIME );  
}

void SpikeDetector::CreatePorts( ) {
    
    data_in_port_ = create_input_port(
        "data",
        MultiChannelDataType<double>( ChannelRange(1, MAX_N_CHANNELS) ),
        PortInPolicy( SlotRange(1) ) );
    
    data_out_port_spikes_ = create_output_port(
        SPIKEDATA_S,
        SpikeDataType( buffer_size_ms_, ChannelRange(1, MAX_N_CHANNELS) ),
        PortOutPolicy( SlotRange(1), RINGBUFFER_SIZE ) );
    
    data_out_port_events_ = create_output_port(
        "events",
        EventDataType(),
        PortOutPolicy( SlotRange(1) ) );
    
    threshold_ = create_writable_shared_state(
        "threshold",
        initial_threshold_,
        Permission::READ,
        Permission::WRITE);
    
    peak_lifetime_ = create_writable_shared_state(
        "peak_lifetime",
        initial_peak_lifetime_,
        Permission::READ,
        Permission::WRITE);
}

void SpikeDetector::CompleteStreamInfo() {
    
    double incoming_stream_rate = data_in_port_->streaminfo(0).stream_rate();
    incoming_buffer_size_samples_ = data_in_port_->slot(0)->streaminfo().datatype().nsamples();
    double incoming_buffer_size_ms =
        incoming_buffer_size_samples_ / data_in_port_->slot(0)->streaminfo().datatype().sample_rate() * 1000;
    
    try {
        check_buffer_sizes_and_log( incoming_buffer_size_ms, buffer_size_ms_,
            strict_time_bin_check_, n_incoming_, name() );
    } catch( std::runtime_error& error ) {
        throw ProcessingStreamInfoError( error.what(), name() );
    }

    n_channels_ = data_in_port_->slot(0)->streaminfo().datatype().nchannels();
    data_out_port_spikes_->streaminfo(0).datatype().Finalize( 
        n_channels_, incoming_stream_rate );
    data_out_port_spikes_->streaminfo(0).Finalize(
        incoming_stream_rate / (incoming_buffer_size_samples_ * n_incoming_) );
    
    data_out_port_events_->streaminfo(0).datatype().Finalize();
    data_out_port_events_->streaminfo(0).Finalize(IRREGULARSTREAM);
}

void SpikeDetector::Prepare( GlobalContext& context ) {
 
    spike_detector_ = new dsp::algorithms::SpikeDetector(
        n_channels_, initial_threshold_, initial_peak_lifetime_ );
    
    if ( invert_signal_ ) {
        inverted_signals_ = new MultiChannelData<double>();
        inverted_signals_->Initialize(
            n_channels_,
            incoming_buffer_size_samples_,
            data_in_port_->slot(0)->streaminfo().datatype().sample_rate());     
    }
}

void SpikeDetector::Process( ProcessingContext& context ) {
    
    decltype(n_incoming_) sample_buffer_counter = 0;
    decltype(data_in_->hardware_timestamp()) hw_timestamp = 0;
    decltype(incoming_buffer_size_samples_) s = 0;
    decltype(n_channels_) c =0;
    decltype(data_in_) signals = nullptr;
    
    std::unique_ptr<EventData> single_spike_event( new EventData("spike") );
    std::unique_ptr<EventData> multiple_spikes_event( new EventData("spikes") );
    
    while (!context.terminated()) {
        
        // update state variables
        spike_detector_->set_threshold( threshold_->get() );
        spike_detector_->set_peak_life_time( peak_lifetime_->get() );
        
        // claim one data bucket and look for spikes
        spike_data_out_ = data_out_port_spikes_->slot(0)->ClaimData( true );
        
        // look for spikes
        while ( sample_buffer_counter < n_incoming_ ) {
            if (!data_in_port_->slot(0)->RetrieveData( data_in_ )) {break;};
            
            // SpikeData will be marked with the the first timestamp of the
            // buffer of samples used for detection
            if (sample_buffer_counter == 0) {
                hw_timestamp = data_in_->hardware_timestamp();
            }
            
            // if spike detection has to be performed on the inverted signal,
            // make a local copy of the inverted signal and use it for spike detection
            if ( invert_signal_ ) {
                for ( s = 0; s < incoming_buffer_size_samples_; ++s ) {
                    for ( c=0; c < n_channels_; ++c ) {
                        inverted_signals_->set_data_sample(s, c,
                            -data_in_->data_sample(s, c));
                    }
                }
                signals = inverted_signals_;
            } else {
                signals = data_in_;
            }

            // detect spikes sample by sample and collect each detected spike
            for ( s = 0; s < incoming_buffer_size_samples_; ++s ) {
                
                if ( spike_detector_->is_spike<double*>(
                    data_in_->sample_timestamp(s), signals->begin_sample(s)) ) {
                    
                    spike_data_out_->add_spike(
                        spike_detector_->amplitudes_detected_spike(),
                        spike_detector_->timestamp_detected_spike() );
                }
            }
            
            // update counters and timestamp data
            ++ sample_buffer_counter;
            spike_data_out_->set_hardware_timestamp( hw_timestamp );
            spike_data_out_->set_source_timestamp();
            
            data_in_port_->slot(0)->ReleaseData();
        }
        
        // publish results on the two ports
        data_out_port_spikes_->slot(0)->PublishData();
        sample_buffer_counter = 0;
        if (spike_data_out_->n_detected_spikes() > 0) {    
            event_data_out_ = data_out_port_events_->slot(0)->ClaimData( false );
            if (spike_data_out_->n_detected_spikes() > 1) {
                event_data_out_->set_event( *multiple_spikes_event );
            } else {
                event_data_out_->set_event( *single_spike_event );
            }
            event_data_out_->set_hardware_timestamp( hw_timestamp );
            
            data_out_port_events_->slot(0)->PublishData();
        }     
    }
}

void SpikeDetector::Postprocess( ProcessingContext& context ) {
    
    LOG(INFO) << name() << ". # spikes detected = " << spike_detector_->nspikes();
    auto spike_rate = spike_detector_->nspikes() /
        (data_out_port_spikes_->slot(0)->nitems_produced()*buffer_size_ms_/1e3);
    LOG(INFO) << name() << ". Spike rate = " << spike_rate << " spikes/s.";
    spike_detector_->reset();
}

void SpikeDetector::Unprepare( GlobalContext& context ) {

    delete spike_detector_; spike_detector_ = nullptr;
    delete inverted_signals_; inverted_signals_ = nullptr;
}