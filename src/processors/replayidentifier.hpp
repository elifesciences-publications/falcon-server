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
 * ReplayIndentifier:
 * identifies the content of replay events from the incoming stream of Likelihood data
 * and the definition of the environment; upon identification of a replay event, 
 * it emits an event labelled with the identified content 
 * 
 * input ports:
 * estimates <LikelihoodData> (1 slot)
 *
 * output ports:
 * events <EventData> (1 slot)
 *
 * exposed states:
 * history <std::uint16_t> - # past likelihood data values used for identifying trajectories
 *
 * exposed methods:
 * none
 *
 * options:
 * initial_history <std::uint16_t> - initial number of history points used for replay identification
 * min_mua <double> - min threshold of MUA (computed over the entire history)
 * for detecting candidate replay events
 * min_peakiness, min_peakiness_history_avg <double> - minimum peakiness 
 * values to be met for a  likelihood to be included in the estimation of a replay trajectory
 * min_slope_diff <double> - maximum difference between two consecutive estimated
 * slopes of a replay trajectory
 * nan_aware <bool> - whether the algorithm will exclude or not NaNs
 * update_interval <int> - number of bins for update, if negative no update
 * interval_test_replay_events <std::uint64_t> - number of bins after which a test
 * replay event will be generated (test mode only)
 * 
 */

#ifndef REPLAYIDENTIFIER_HPP
#define	REPLAYIDENTIFIER_HPP

#include "../graph/iprocessor.hpp"
#include "environment/environment.hpp"
#include "../data/likelihooddata.hpp"
#include "../data/eventdata.hpp"

template <typename T1, typename T2>
void update_history( T1& container, T2 new_value);

struct ReplayIdentificationParameters {
    std::uint16_t initial_history;
    double min_mua; // integrated over history time bins
    double min_mua_std; // will be used to compute min_mua
    size_t half_integration_window;
    double min_peakiness;
    decltype(min_peakiness) min_peakiness_history_avg; // integrated over the number of history points
    double max_2nd_der;
    
    decltype(initial_history) slope_history() const;
    
    const decltype(initial_history) DEFAULT_HISTORY = 3;
    const decltype(min_mua_std) DEFAULT_MINIMUM_MUA_STD = 3.5;
    const decltype(min_peakiness) DEFAULT_MINIMUM_PEAKINESS = 0.2;
    const decltype(min_peakiness_history_avg) DEFAULT_MINIMUM_PEAKINESS_HISTORY_AVG = 0.65; // peakiness averaged over the number of history points
    const decltype(max_2nd_der) DEFAULT_MAX_2ND_DER = 6.5;
    const decltype(half_integration_window) DEFAULT_HALF_INTEGRATION_WINDOW = 4;
    
    bool valid_parameters();
    
    void log_parameters( std::string processor_name ) const;
    
    void reset();
};

struct ReplayIdentificationRegister {
    
    std::valarray<double> mua;
    std::vector<std::uint16_t> peak_grid_index;
    std::valarray<double> peakiness;
    std::valarray<double> slope;
    std::vector<std::size_t> history_indices;
    double min_abs_2nd_derivative;
    
    std::uint16_t history() const {return history_;}
    
private:
    std::valarray<double> slope_differences_;
    std::uint16_t history_;
    
public:
    double mua_history_avg() const; // == S3
    double peakiness_history_avg( bool nan_aware=false ) const;
    std::valarray<double>& slope_differences(); // 2nd derivative
    void reset(std::uint16_t history);
};

class ReplayIdentificationCounter {
public:
    ReplayIdentificationCounter( std::vector<Content> contents );
    
    void add_event();
    void add_event(Content event);
    
    unsigned int n_all_replays() const;
    void reset(std::uint8_t n_contents);
    void print_content_counters(std::string processor_name) const;
    bool all_replays_with_content() const;
    
private:
    std::vector<Content> contents_;
    unsigned int all_replay_counter_;
    std::vector<unsigned int> content_counters_;
};

class ReplayIdentifier : public IProcessor {
public:
    virtual void Configure( const YAML::Node  & node, const GlobalContext& context ) override;
    virtual void CreatePorts( ) override;
    virtual void CompleteStreamInfo() override;
    virtual void Prepare( GlobalContext& context ) override;
    virtual void Preprocess( ProcessingContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;
    virtual void Unprepare( GlobalContext& context ) override;
    
protected:
    void load_grid( size_t content_node );
    void check_history( int history ) const;
    double compute_peakiness(LikelihoodData* likelihood,
        std::size_t peak_grid_index, std::size_t half_integration_window);
    void save_intermediate_values();
    void update_user();
    void compute_mua_statistics(std::string path_to_mua, double& mean, double& stdev);
    
protected:
    PortIn<LikelihoodDataType>* data_in_port_;
    PortOut<EventDataType>* data_out_port_;
    ReadableState<std::uint16_t>* history_;
    
    std::string path_to_environment_;
    std::string path_to_mua_file_;
    Environment* environment_;
    int update_interval_;
    unsigned int interval_test_replay_events_;
    bool first_print_done_;
    bool save_intermediate_values_;
    
    ReplayIdentificationParameters params_;
    ReplayIdentificationRegister reg_;
    ReplayIdentificationCounter* counter_;
    std::uint64_t n_bins_replay_unknown_content_;
    bool nan_aware_;
    std::uint64_t n_processed_bins_;
    std::uint64_t n_test_replays_;
    bool latency_test_;
    
    std::size_t win_half1_;
    std::size_t win_half2_;
    double peakiness_acc_;
    
public:
    const bool DEFAULT_NAN_AWARENESS = false;
    const decltype(update_interval_) DEFAULT_UPDATE_INTERVAL = 300;
    const decltype(interval_test_replay_events_)
        DEFAULT_INTERVAL_TEST_REPLAY_EVENTS = 20;
    const decltype(save_intermediate_values_) DEFAULT_SAVE_INTERMEDIATE_VALUES = true;
    
protected:
    struct intermediate_values_names { // for testing purposes
        std::string mua = "mua";
        std::string mua_avg = "mua_avg";
        std::string peak_gridindex = "peakgridindex";
        std::string peakiness = "peakiness";
        std::string peakiness_avg = "peakiness_avg";
        std::string slope_values = "slope_values";
        std::string slope_diffs = "slope_differences";
    } intermediate_values_names;
};

#include "replayidentifier.ipp"
#endif	// replayidentifier.hpp

