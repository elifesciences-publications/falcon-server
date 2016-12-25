#include "spikestreamer.hpp"

#include <chrono>
#include <thread>

#include "g3log/src/g2log.hpp"
#include "npyreader/npyreader.h"
#include "utilities/math_numeric.hpp"
#include "utilities/string.hpp"


constexpr double SpikeStreamer::DEFAULT_BUFFERSIZE_MS;
constexpr double SpikeStreamer::DEFAULT_SAMPLE_RATE;
constexpr double SpikeStreamer::DEFAULT_STREAMING_RATE;
constexpr double SpikeStreamer::HIGH_DATA_STREAM_RATE;


void SpikeStreamer::Configure(const YAML::Node& node, const GlobalContext& context) {
    
    path_to_spikes_ = context.resolve_path(
        node["path_to_spikes"].as<std::string>( ) );
    path_to_spike_times_ = context.resolve_path(
        node["path_to_spike_times"].as<std::string>( ) );
    path_to_initial_times_ = context.resolve_path(
        node["path_to_initial_times"].as<std::string>( ) );
    
    buffer_size_ms_ = node["buffer_size_ms"].as<double>( DEFAULT_BUFFERSIZE_MS );
    sample_rate_ = node["sample_rate"].as<double>( DEFAULT_SAMPLE_RATE );
    streaming_rate_ = node["streaming_rate"].as<double>( DEFAULT_STREAMING_RATE );
    
    n_channels_ = node["n_channels"].as<decltype(n_channels_)>( NO_CHANNEL_NUMBER );
    if ( n_channels_ == NO_CHANNEL_NUMBER ) {
        path_to_nchannels_ = context.resolve_path(
            node["path_to_nchannels"].as<std::string>( ) );
        int32_t* n_channels = nullptr;
        FILE* fp_nchannels = nullptr;
        complete_path( path_to_nchannels_, name(), "npy" );
        if ( (fp_nchannels = fopen(path_to_nchannels_.c_str(), "r")) == nullptr ) {
            throw ProcessingPrepareError("Cannot open the NPY file. Check the file path "
                + path_to_nchannels_, name());
        }
        if ( (n_channels = get_1D_array_int32( fp_nchannels ) ) == nullptr ) {
            throw ProcessingPrepareError(
                "Number of channels was not read correctly.", name());
        }
        n_channels_ = *n_channels;
        if ( n_channels_ < 0 ) {
            throw ProcessingConfigureError("Number of channels must be a positive number");
        } 
    } else {
        path_to_nchannels_ = 
            node["path_to_nchannels"].as<std::string>( "empty" );
        LOG_IF( WARNING, ( path_to_nchannels_ != "empty" )) << name() <<
            ". Path to n_channels will be ignored because it was explicitly defined.";
    }
    LOG(INFO) << name() << ". Number of spike amplitudes of the stream = " <<
        n_channels_ << ".";
    
    if ( sample_rate_ <= 0 ) {
       throw ProcessingConfigureError("Sample rate must be a positive number.", name());
    }
    if ( buffer_size_ms_ == 0 ) {
        throw ProcessingConfigureError("Buffer size cannot be zero.", name());
    }
    if ( streaming_rate_ <= 0 ) {
        throw ProcessingConfigureError(
            "Streaming rate must be a positive number.", name());
    }
    if ( streaming_rate_ * sample_rate_ / (buffer_size_ms_*1e3) > HIGH_DATA_STREAM_RATE ) {
        LOG(WARNING) << name() << ". SpikeData stream rate is very high." <<
            " Downstream processor might not be able to keep up.";
    }
}

void SpikeStreamer::CreatePorts() {
    
    data_out_port_ = create_output_port(
        "spikes",
        SpikeDataType( buffer_size_ms_, ChannelRange(1, n_channels_) ),
        PortOutPolicy( SlotRange(1), RINGBUFFER_SIZE ) );
}

void SpikeStreamer::CompleteStreamInfo() {

    data_out_port_->streaminfo(0).datatype().Finalize( n_channels_, sample_rate_ );
    data_out_port_->streaminfo(0).Finalize( sample_rate_ * buffer_size_ms_ * 1e-3 );
}

void SpikeStreamer::Prepare( GlobalContext& context ) {

    uint32_t n_amplitudes = 0;
    size_t n_buffers_per_bin = 0;
    auto buffer_size_sec = buffer_size_ms_ * 1e-3;
    FILE* fp_amplitudes;
    FILE* fp_times;
    FILE* fp_initial_times;
    
    complete_path( path_to_spikes_, name(), ".npy" );
    complete_path( path_to_spike_times_, name(), ".npy" );
    complete_path( path_to_initial_times_, name(), ".npy" );
    
    // open the NYP files and extract the number of spikes, making sure that
    // there is consistency between the two files
    if ( (fp_amplitudes = fopen(path_to_spikes_.c_str(), "r")) == nullptr ) {
        throw ProcessingPrepareError("Cannot open the NPY file. Check the file path "
            + path_to_spikes_, name());
    }
    if ( (fp_times = fopen(path_to_spike_times_.c_str(), "r")) == nullptr ) {
        throw ProcessingPrepareError("Cannot open the NPY file. Check the file path "
            + path_to_spike_times_, name());
    }
    if ( get_1D_array_len(fp_times, &n_spikes_) != 0 ) {
        throw ProcessingPrepareError(
            "Cannot read the length of the spike times array from the NPY file.",
            name());
    } else {
        LOG(INFO) << name() << ". The NPY file contains " << n_spikes_ << " spikes.";
    }
    if ( get_1D_array_len(fp_amplitudes, &n_amplitudes) != 0 ) {
        throw ProcessingPrepareError(
            "Cannot read the length of the spike amplitudes array from the NPY file.",
            name());
    }
    if ( n_spikes_ != (n_amplitudes / n_channels_) ) {
        throw ProcessingPrepareError(
            "The two files do not have a consistent number of spikes", name());
    }
    LOG(INFO) << name() << ". Consistency check passed.";
    
    // load the raw spike data
    if ( (loaded_spike_amplitudes_ = get_1D_array_f64( fp_amplitudes ) ) == nullptr ) {
        throw ProcessingPrepareError(
            "Spike amplitude data was not loaded correctly.", name());
    }
    if ( (loaded_spike_times_ = get_1D_array_f64( fp_times ) ) == nullptr ) {
        throw ProcessingPrepareError(
            "Spike times were not loaded correctly.", name());
    }
    
    LOG(INFO) << name() <<
        ". Spike amplitudes and times were successfully loaded from the files.";
    
    // load the initial times of the binned spikes
    if ( (fp_initial_times = fopen(path_to_initial_times_.c_str(), "r") ) == nullptr ) {
        throw ProcessingPrepareError("Cannot open the NPY file. Check the file path "
            + path_to_initial_times_, name());
    }
    if ( get_1D_array_len(fp_initial_times, &n_bins_) != 0 ) {
        throw ProcessingPrepareError(
            "Cannot read the length of the initial times array from the NPY file.",
            name());
    } else if ( n_bins_ != 1 ) {
        LOG(INFO) << name() << ". Loaded data is divided into " << n_bins_ << " bins."; 
    }
    if ( (loaded_initial_times_ = get_1D_array_f64( fp_initial_times ) ) == nullptr ) {
        throw ProcessingPrepareError( "initial times vector was not loaded correctly.",
        name());
    }
    
    // compute number of spike data items to stream and
    // prepare the time limits of each SpikeData element
    if ( n_bins_ == 1) {
        n_packets_to_stream_ = std::trunc(
            ( loaded_spike_times_[n_spikes_ - 1] - loaded_initial_times_[0] ) /
            buffer_size_sec ) + 1;
        time_limits_ = linspace( loaded_initial_times_[0],
                                loaded_spike_times_[n_spikes_ - 1], n_packets_to_stream_ );
    } else {
        auto bin_size = loaded_initial_times_[1] - loaded_initial_times_[0];
        if ( !compare_doubles( remainder( bin_size, buffer_size_sec ), 0, ERROR_ ) ) {
            throw ProcessingPrepareError( std::string("Selected buffer size must be ") 
                + "an exact multiple of the bin size (bin_size = " + std::to_string(bin_size) +
                " s; requested buffer size = " + std::to_string(buffer_size_sec) + " s).");
        }
        n_buffers_per_bin = std::round( bin_size / buffer_size_sec );
        n_packets_to_stream_ = n_bins_ * n_buffers_per_bin;
        
        time_limits_.reserve( n_packets_to_stream_ );
        for (decltype(n_bins_) i = 0; i < n_bins_; ++i) {
            for (decltype(n_buffers_per_bin) j = 0; j < n_buffers_per_bin ; ++j) {
                time_limits_.push_back( loaded_initial_times_[i] + j * buffer_size_sec );
            }   
        }
    }
    LOG(INFO) << name() << ". " << n_packets_to_stream_ << " packets of SpikeData "
        << "will be streamed on the output port.";
    
    fclose(fp_amplitudes); fp_amplitudes = nullptr;
    fclose(fp_times); fp_times = nullptr;
    fclose(fp_initial_times); fp_initial_times = nullptr;
}

void SpikeStreamer::Process(ProcessingContext& context) {
    
    SpikeData* data = nullptr;
    auto delay = std::chrono::microseconds(
        static_cast<unsigned int>( 1e6 / streaming_rate_ ) );
    decltype(n_spikes_) n = 0;
  
    // the first value is not a boundary to the next temporal bin    
    unsigned int time_limit_cursor = 1;
    
    while ( !context.terminated() &&
            data_out_port_->slot(0)->nitems_produced() < n_packets_to_stream_ ) {
        
        data = data_out_port_->slot(0)->ClaimData( true );
        assert( data->n_detected_spikes() == data->ts_detected_spikes().size());
        assert( data->n_detected_spikes() == 0 );
        
        
        while ( loaded_spike_times_[n] < time_limits_[time_limit_cursor] && 
                n < n_spikes_) {
            
            data->add_spike( &loaded_spike_amplitudes_[n * n_channels_],
                static_cast<uint64_t>( std::round( loaded_spike_times_[n]*1e6 ) ) );
            ++ n;
        }
        
        data->set_hardware_timestamp(
            static_cast<uint64_t>(std::round( time_limits_[time_limit_cursor-1]*1e6 )));
        data->set_source_timestamp();
        ++ time_limit_cursor;
        
        if ( data->n_detected_spikes() != data->ts_detected_spikes().size() ) {
            
            LOG(ERROR) << name() << ". data->n_detected_spikes() = " <<
            data->n_detected_spikes() << ". data->ts_detected_spikes().size() = "
            << data->ts_detected_spikes().size();
        }
        data_out_port_->slot(0)->PublishData();
        
        std::this_thread::sleep_for( delay );
    }  
    
    // this log goes here (and in PostProcess) so that the STOP commands
    // can act as an end-of-stream signal
    LOG(INFO) << name()<< ". Streamed " << data_out_port_->slot(0)->nitems_produced()
        << " data packets. PRESS 's' to STOP THE GRAPH";
}

void SpikeStreamer::Unprepare( GlobalContext& context ) {

    free(loaded_spike_amplitudes_); loaded_spike_amplitudes_ = nullptr;
    free(loaded_spike_times_); loaded_spike_times_ = nullptr;
    if ( n_bins_ != 1 ) {
        free(loaded_initial_times_); loaded_initial_times_ = nullptr;
    }
}
