/* SpikeDetector: detects spikes on any of the incoming MultiChannelData stream;
 * sends SpikeData on the output port spikes and an event "spike"/"spikes"
 *  on the output port events if one or more spikes have been detected in the received buffer
 * 
 * input ports:
 * data <MultiChannelData> (1 slot)
 *
 * output ports:
 * spikes <SpikeData> (1 slot)
 * events <EventData> (1 slot)
 *
 * exposed states:
 * threshold <double> - threshold that a single channel must cross
 * peak_lifetime <unsigned int> - number of samples that will be used to look for a peak;
 *
 *
 * exposed methods:
 * none
 *
 * options:
 * buffer_size <double> - amount of data that will be used to look for spikes [ms]
 * strict_time_bin_check <bool> - whether the buffer size will be strictly or loosely checked
 * for compatibility with the upstream processor
 * threshold <double> - initial threshold state
 * invert_signal <bool> - whether the signal does (true) or does not (false) need
 * to be inverted when detecting spikes
 * peak_lifetime <unsigned int> - initial peak_lifetime value
 * 
 */

#ifndef SPIKEDETECTOR_H
#define SPIKEDETECTOR_H

#include "../graph/iprocessor.hpp"
#include "dsp/algorithms.hpp"
#include "../data/spikedata.hpp"
#include "../data/eventdata.hpp"
#include "../data/multichanneldata.hpp"

class SpikeDetector : public IProcessor {

public:
    virtual void Configure( const YAML::Node  & node, const GlobalContext& context) override;
    virtual void CreatePorts( ) override;
    virtual void CompleteStreamInfo() override;
    virtual void Prepare( GlobalContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override; 
    virtual void Unprepare( GlobalContext& context ) override;

protected:
    PortIn<MultiChannelDataType<double>>* data_in_port_;
    PortOut<SpikeDataType>* data_out_port_spikes_;
    PortOut<EventDataType>* data_out_port_events_;
    
    WritableState<double>* threshold_;
    WritableState<unsigned int>* peak_lifetime_;
    
    unsigned int n_channels_;
    double initial_threshold_;
    unsigned int initial_peak_lifetime_;
    bool invert_signal_;
    double buffer_size_ms_;
    bool strict_time_bin_check_;
    size_t n_incoming_;
    size_t incoming_buffer_size_samples_;
    
    dsp::algorithms::SpikeDetector* spike_detector_;
    
    MultiChannelData<double>* data_in_;
    MultiChannelData<double>* inverted_signals_;
    SpikeData* spike_data_out_;
    EventData* event_data_out_;
    
    uint64_t n_streamed_events_;
    
public:
    const decltype(initial_threshold_) DEFAULT_THRESHOLD = 60.0;
    const decltype(invert_signal_) DEFAULT_INVERT_SIGNAL = true;
    const decltype(initial_peak_lifetime_) DEFAULT_PEAK_LIFETIME = 8;
    const decltype(buffer_size_ms_) DEFAULT_BUFFER_SIZE_MS = 0.5;
    const decltype(strict_time_bin_check_) DEFAULT_STRICT_TIME_BIN_CHECK = true;
    const decltype(n_channels_) MAX_N_CHANNELS = 8;
    
protected:
    const int RINGBUFFER_SIZE = 1e5;
    
};

#endif // spikedetector.h
