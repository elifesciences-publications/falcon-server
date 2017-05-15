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

/* RippleDetector: detects ripples in a MultiChannelData stream and
 * emits an ripple event in response
 * 
 * input ports:
 * data <MultiChannelData> (1 slot)
 *
 * output ports:
 * events <EventData> (1 slot)
 *
 * exposed states:
 * threshold_dev <double> - threshold that needs to be crossed (read-only)
 * mean <double> - signal mean (read-only)
 * deviation <double> - signal deviation (read-only)
 * threshold_deviation <double> - multiplier for threshold (# of deviations)
 * detection_lockout_time_ms <unsigned int> - refractory period after threshold
 *   crossing detection that is not considered for updating of statistics
 *   and for detecting events
 * stream_events <bool> - enable/disable ripple event output
 * stream_statistics <bool> - enable/disable streaming of ripple detection statistics
 *
 * exposed methods:
 * none
 *
 * options:
 * threshold_dev <double> - default threshold multiplier
 * smooth_time <double> - integration time for signal statistics
 * detection_lockout_time_ms <double> - default lockout time ( no 
 * stream_events <bool> - default enable state for streaming events
 * stream_statistics <bool> - enable streaming of statistics
 * statistics_buffer_size <double> - Buffer size (in seconds) for
 *   statistics output buffers
 * statistics_downsample_factor <unsigned int> - downsample factor of streamed
 *   statistics signal
 * 
 */
 
#ifndef RIPPLEDETECTOR_HPP
#define RIPPLEDETECTOR_HPP

#include "../graph/iprocessor.hpp"

#include "../data/multichanneldata.hpp"
#include "../data/eventdata.hpp"
#include "dsp/algorithms.hpp"

#include <memory>

class RippleDetector : public IProcessor {

public:
    virtual void Configure( const YAML::Node & node, const GlobalContext& context) override;
    virtual void CreatePorts( ) override;
    virtual void CompleteStreamInfo( ) override;
    virtual void Preprocess( ProcessingContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;
    
protected:
    double compute_value( MultiChannelData<double>* data_in, unsigned int sample );
    
protected:
    PortIn<MultiChannelDataType<double>>* data_in_port_;
    PortOut<EventDataType>* event_out_port_;
    PortOut<MultiChannelDataType<double>>* stats_out_port_;
    
    WritableState<double>* threshold_;
    WritableState<double>* signal_mean_;
    WritableState<double>* signal_dev_;
    WritableState<bool>* ripple_;
    
    double initial_threshold_dev_;
    unsigned int initial_detection_lockout_time_;
    bool default_stream_events_;
    double initial_smooth_time_;
    bool initial_stats_out_;
    bool use_power_;
    
    ReadableState<decltype(initial_threshold_dev_)>* threshold_dev_;
    ReadableState<decltype(initial_detection_lockout_time_)>*
        detection_lockout_time_;
    ReadableState<decltype(default_stream_events_)>* stream_events_;
    ReadableState<decltype(initial_smooth_time_)>* smooth_time_;
    ReadableState<decltype(initial_stats_out_)>* stats_out_;
    
    bool stats_out_enabled_;
    double stats_buffer_size_;
    std::uint64_t stats_nsamples_;
    unsigned int stats_downsample_factor_;
    
    std::uint64_t block_;
    std::uint64_t burn_in_;
    double sample_rate_;
    
    double acc_;
    
    std::unique_ptr<dsp::algorithms::RunningMeanMAD> running_statistics_;
    std::unique_ptr<dsp::algorithms::ThresholdCrosser> threshold_detector_;

public:
    const decltype(initial_threshold_dev_) DEFAULT_THRESHOLD_DEV = 6.0;
    const decltype(initial_smooth_time_) DEFAULT_SMOOTH_TIME = 10.0;
    const decltype(initial_detection_lockout_time_)
        DEFAULT_DETECTION_LOCKOUT_TIME = 30;
    const decltype(default_stream_events_) DEFAULT_STREAM_EVENTS = true;
    const decltype(initial_stats_out_) DEFAULT_STREAM_STATISTICS = true;
    const decltype(stats_buffer_size_) DEFAULT_STATISTICS_BUFFER_SIZE = 0.5; // in seconds
    const decltype(stats_downsample_factor_) DEFAULT_STATISTICS_DOWNSAMPLE_FACTOR = 1;
    
public:
    // configure options names ( keep common between options and states )
    const std::string THRESHOLD_DEV_S = "threshold_dev";
    const std::string SMOOTH_TIME_S = "smooth_time";
    const std::string DETECTION_LOCKOUT_TIME_S = "detection_lockout_time_ms";
    const std::string STREAM_EVENTS_S = "stream_events";
    const std::string STREAM_STATISTICS_S = "stream_statistics";
    const std::string STATISTICS_BUFFER_SIZE_S = "statistics_buffer_size";
    const std::string STATISTICS_DOWNSAMPLE_FACTOR_S = "statistics_downsample_factor";
    
protected:
    const unsigned int N_STATS_OUT = 2;
};

#endif // rippledetector.hpp
