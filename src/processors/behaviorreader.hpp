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
 * BehaviorReader: it computes the behavioral measurment of interset from the incoming
 * VideotrackData and from an input matrix, it transforms the 
 * 2D info of the videotrack into a 1D measurement of position, it also computes
 * speed in the linearized space to complete the beha 
 * 
 * input ports:
 * vt_data <VideoTrackData> (1 slot)
 * 
 * output ports:
 * behavior <BehaviorData> (1 slot)
 * 
 * exposed methods:
 * none
 * 
 * options:
 * path_to_linearization_matrix <string> - filepath to NPY file containing the matrix 
 * of linearized positions; negative values indicates out-of-environment points
 * use_single_coordinate <char> - if 'x' or 'y' linearization will be performed
 * by using only the x- or y- coordinate of the VT data; only used if no path to
 * linearization matrix was entered
 * pixel_to_cm <double> - conversion factor in pixel/cm, if negative or zero behavior
 * unit is set to 'pixel' instead of 'cm'
 * n_ma_position <unsigned int> - number of samples used for smoothing position
 * n_ma_speed <unsigned int> - number of samples used for smoothing speed
 * batch_size <unsiged int> -number of videotrack samples used to compute speed
 * update_interval <unsigned int> - number of seconds after which updates
 * about the status of the network stream should be logged
 */

#ifndef BEHAVIOR_READER_HPP
#define	BEHAVIOR_READER_HPP

#include "dsp/behavior_algorithms.hpp"
#include "../graph/iprocessor.hpp"
#include "../data/videotrackdata.hpp"
#include "../data/behaviordata.hpp"

enum LinearizationMode {
    MATRIX,
    X,
    Y
};

class BehaviorReader : public IProcessor {
    
public:
    virtual void Configure( const YAML::Node& node, const GlobalContext& context) override;
    virtual void CreatePorts() override;
    virtual void CompleteStreamInfo() override;
    virtual void Prepare( GlobalContext& context ) override;
    virtual void Preprocess( ProcessingContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;
    virtual void Unprepare( GlobalContext& context ) override;
    
protected:
    void log_out_of_maze();    
    void save_intermediate_signals();

protected:
    PortIn<VideoTrackDataType>* data_in_port_;
    PortOut<BehaviorDataType>* data_out_port_;
    
    std::string path_to_linearization_matrix_;
    char use_single_coordinate_;
    double pixel_to_cm_;
    decltype(pixel_to_cm_) conversion_factor_;
    
    double** linearization_matrix_;
    LinearizationMode linearization_mode_;
    
    std::unique_ptr<dsp::behavior_algorithms::SpeedCalculator> speed_calculator_;
    unsigned int n_ma_position_;
    unsigned int n_ma_speed_; 
    unsigned int batch_size_;
    
    std::uint64_t n_received_;
    decltype(n_received_) n_outside_environment_;
    unsigned int update_interval_;
    
public:
    const decltype(update_interval_) DEFAULT_UPDATE_INTERVAL_SEC = 20;
    const decltype(n_ma_position_) DEFAUL_N_MA_POSITION = 11;
    const decltype(n_ma_speed_) DEFAULT_N_MA_SPEED = 5;
    const decltype(batch_size_) DEFAULT_BATCH_SIZE = 50;
    
protected:
    const std::string SMOOTH_POSITION_FILENAME = "smoothed_position";
    const std::string NON_SMOOTHED_SPEED_FILENAME = "unsmoothed_speed";
    const decltype(use_single_coordinate_) DEFAULT_USE_SINGLE_COORDINATE = 'n';

};

#endif	// behaviorreader.hpp

