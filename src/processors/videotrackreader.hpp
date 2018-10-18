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
 * VideotrackReader: read videotrack data packets from ZMQ socket streamed
 * by Cheetah Netcom client
 * 
 * output ports:
 * data <VideoTrackData> (1 slot)
 *
 * exposed methods:
 * none
 * 
 * options:
 * ip_address <string> - IP address of the machine
 * vt_id <uint16_t> - id of the VT object in Cheetah
 * n_max_consecutive_occlusions <unsigned int> - max number of consecutive
 * detected occlusions that should be replaced with the last valid value 
 * update_interval <unsigned int> - number of seconds after which updates
 * about the status of the network stream should be logged
 */

#ifndef VIDEOTRACK_READER_HPP
#define	VIDEOTRACK_READER_HPP

#include "neuralynx/nlx.hpp"
#include "../graph/iprocessor.hpp"
#include "../data/videotrackdata.hpp"

#include "utilities/time.hpp"

#include <zmq.hpp>
#include <memory>

class VideoTrackReader : public IProcessor {
    
public:
    virtual void Configure( const YAML::Node& node, const GlobalContext& context) override;
    virtual void CreatePorts() override;
    virtual void CompleteStreamInfo() override;
    virtual void Prepare( GlobalContext& context) override;
    virtual void Preprocess( ProcessingContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;
    
protected:
    bool check_validity( VideoRec* vt_record, std::uint16_t vt_id );    

protected:
    PortOut<VideoTrackDataType>* data_out_port_;

    std::string ip_address_;
    std::uint16_t vt_id_;
    std::array<std::int32_t, 2> no_videotrack_coordinates_; // VT coordinates of no tracking
    std::array<std::int32_t, 3> last_nonoccluded_measures_;
    unsigned int n_occlusions_;
    decltype(n_occlusions_) n_max_consecutive_occlusions_;
    
    bool is_valid_;
    ErrorNLXVT::Code error_code_;
    bool out_of_order_;
    
    TimePoint first_packet_arrival_time_;
    std::uint64_t update_interval_;
    std::uint64_t packet_counter_;
    std::unique_ptr<zmq::socket_t> socket_;
    
public:
    const decltype(vt_id_) DEFAULT_VT_ID = 0;
    const std::uint64_t NO_TIMESTAMP =
        std::numeric_limits<std::uint64_t>::max();
    const double DEFAULT_UPDATE_INTERVAL_SEC = 20;
    decltype(no_videotrack_coordinates_) DEFAUL_NO_VIDEOTRACK = {{0, 0}}; 
    const decltype(n_max_consecutive_occlusions_)
        DEFAULT_N_MAX_CONSECUTIVE_OCCLUSIONS = 10;

};

#endif	// videotrackreader.hpp

