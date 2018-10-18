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

/* 
 * VideotrackDataFileStreamer: streams VideoTrack Data loaded from three NPY files.
 * If no file is present, data will be streamed with a null values; at least one file
 * must be specified.
 * 
 * input ports:
 * none
 * 
 * output ports:
 * vt_data <VideotrackData> (1 slot)
 * 
 * exposed states:
 * none
 *
 * exposed methods:
 * none
 * 
 * options:
 * path_to_x <string> - full path of the NPY file with the x coordinate values
 * (NPY file must contain int32 data)
 * path_to_y <string> - full path of the NPY file with the y coordinate values
 * (NPY file must contain int32 data)
 * path_to_angle <string> - full path the NPY file with the angle values
 * of spikes for each time bin (NPY file must contain int32 data)
 * path_to_occlusions <string> - full path of the NPY file with the occlusion mask
 * (NPY file must contain int32 data)
 * sample_rate <double> - sample rate of the simulated video stream that is being
 * loaded from file
 * streaming_rate <double> - (approximate) streaming rate of the each generated VideoTrackData item 
 * resolution <vector<int>> - resolution of the camera of the simulated video stream
 * that is being loaded from file
 * initial_timestamp <uint64_t> - timestamp of the first streamed VideoTrackData packet
 */

#ifndef VIDEOTRACKDATA_FILESTREAMER_HPP
#define	VIDEOTRACKDATA_FILESTREAMER_HPP

#include "../graph/iprocessor.hpp"
#include "neuralynx/nlx.hpp"
#include "npyreader/npyreader.h"
#include "../data/videotrackdata.hpp"

class VideoTrackDataFileStreamer : public IProcessor {
    
public:
    virtual void Configure( const YAML::Node& node, const GlobalContext& context) override;
    virtual void CreatePorts() override;
    virtual void CompleteStreamInfo() override;
    virtual void Prepare( GlobalContext& context) override;
    virtual void Preprocess( ProcessingContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;
    virtual void Unprepare( GlobalContext& context ) override;  

protected:
    PortOut<VideoTrackDataType>* data_out_port_;
    
    std::string path_to_x_;
    std::string path_to_y_;
    std::string path_to_angle_;
    std::string path_to_occlusions_;
    bool load_x_;
    bool load_y_;
    bool load_angle_;
    bool load_occlusions_;
    std::int32_t* loaded_x_values_;
    std::int32_t* loaded_y_values_;
    std::int32_t* loaded_angle_values_;
    std::int32_t* loaded_occlusions_;
    
    double sample_rate_;
    double streaming_rate_;
    std::array<std::int32_t, 2> resolution_;
    
    uint32_t n_packets_to_stream_;
    uint64_t initial_timestamp_;
    std::vector<decltype(initial_timestamp_)> generated_hw_timestamps_;


public:
    const decltype(sample_rate_) DEFAULT_SAMPLE_RATE = NLX_VIDEO_SAMPLING_FREQUENCY;
    const decltype(streaming_rate_) DEFAULT_STREAMING_RATE = NLX_VIDEO_SAMPLING_FREQUENCY;
    const decltype(initial_timestamp_) DEFAULT_INITIAL_TS = 0;
    const std::string NULL_PATH = "";
};

#endif	// videotrackdatafilestreamer.hpp

