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

#include "replayidentifier.hpp"
#include "g3log/src/g2log.hpp"
#include "utilities/math_numeric.hpp"
#include "npyreader/npyreader.h"

void ReplayIdentificationParameters::reset() {
    
    initial_history = 0;
    min_mua = 0;
    half_integration_window = 0;
    min_peakiness = 0;
    min_peakiness_history_avg = 0;
    max_2nd_der = 0;
}

bool ReplayIdentificationParameters::valid_parameters() {
    
    if (max_2nd_der <= 0) return false;
    return true;
}

void ReplayIdentificationParameters::log_parameters(std::string processor_name) const {
    
    LOG(DEBUG) << processor_name <<
        ". initial_history = " << static_cast<int>(this->initial_history);
    LOG(DEBUG) << processor_name <<
        " . min_mua = " << this->min_mua;
    LOG(DEBUG) << processor_name <<
        "min_peakiness = " << this->min_peakiness;
    LOG(DEBUG) << processor_name <<
        "min_peakiness_history_avg = " << this->min_peakiness_history_avg;
    LOG(DEBUG) << processor_name <<
        "min_abs_slope_diff = " << this->max_2nd_der;
    LOG(DEBUG) << processor_name <<
        ". half_integration_window = " << this->half_integration_window;
}

decltype(ReplayIdentificationParameters::initial_history)
ReplayIdentificationParameters::slope_history() const {
    
    return initial_history-1;
}

double ReplayIdentificationRegister::mua_history_avg() const {
    
    return mua.sum() / history_;
}

double ReplayIdentificationRegister::peakiness_history_avg( bool nan_aware ) const {
    
    if ( nan_aware ) {
        return nan_mean( std::begin(peakiness), std::end(peakiness), history_ );
    }
    assert ( peakiness.size() == history_ );
    return peakiness.sum() / history_ ; 
}

std::valarray<double>& ReplayIdentificationRegister::slope_differences() { // 2nd derivative
        
    for (unsigned int i = 1; i < slope.size(); ++ i) {
        this->slope_differences_[i-1] = slope[i] - slope[i-1];
    }

    return slope_differences_;
}

void ReplayIdentificationRegister::reset( std::uint16_t history ) {
    
    assert (history != 0);
    
    history_indices.resize( history );
    for (std::size_t i=0; i < history_indices.size(); ++ i) {
        history_indices[i] = i;
    }

    mua.resize( history );
    peak_grid_index.resize( history );
    peakiness.resize( history );
    slope.resize( history - 1 ); // the algorithm uses a 1-bin shorter history for slope 
    slope_differences_.resize( slope.size() - 1 );
    history_ = history;
}


 ReplayIdentificationCounter::ReplayIdentificationCounter( std::vector<Content> contents ) {
        
    contents_ = contents;
    reset( contents.size() );
}
    
void ReplayIdentificationCounter::add_event() {
    
    ++ all_replay_counter_;
}

void ReplayIdentificationCounter::add_event(Content event) {
    
    ++ all_replay_counter_;
    for (size_t i=0; i < contents_.size(); ++ i) {
        if (event == contents_[i]) {
            ++ content_counters_[i];
        }
    }
}

unsigned int ReplayIdentificationCounter::n_all_replays() const {
    
    return all_replay_counter_;
}

void ReplayIdentificationCounter::reset( std::uint8_t n_contents ) {
    
    all_replay_counter_ = 0;
    content_counters_.assign(n_contents, 0);
}

void ReplayIdentificationCounter::print_content_counters( std::string processor_name ) const {
    
    assert ( content_counters_.size() == contents_.size() );
    for (size_t i=0; i < content_counters_.size(); ++i) {
        LOG(INFO) << processor_name <<  ". There were: " << 
            content_counters_[i] << " bins with replay events of content " <<
            contents_[i].name() << ".";
    }
}

bool ReplayIdentificationCounter::all_replays_with_content() const {
    
    unsigned int sum_of_elems = 0;
    std::for_each(content_counters_.begin(),content_counters_.end(),[&](unsigned int n) {
        sum_of_elems += n;
    });
    return sum_of_elems == all_replay_counter_;
}
    
void ReplayIdentifier::Configure( const YAML::Node  & node, const GlobalContext& context) {
    
    params_.initial_history = node["history"].as<decltype(params_.initial_history)>(
        params_.DEFAULT_HISTORY );
    check_history( params_.initial_history );
    
    nan_aware_ = node["nan_aware"].as<decltype(nan_aware_)>( DEFAULT_NAN_AWARENESS );
    
    params_.min_mua_std = node["min_MUA_std"].as<decltype(params_.min_mua_std)>(
        params_.DEFAULT_MINIMUM_MUA_STD );
    params_.half_integration_window =
        node["half_integration_window"].as<decltype(params_.half_integration_window)>(
        params_.DEFAULT_HALF_INTEGRATION_WINDOW );
    params_.min_peakiness = node["min_peakiness"].as<decltype(params_.min_peakiness)>(
        params_.DEFAULT_MINIMUM_PEAKINESS );
    params_.min_peakiness_history_avg =
        node["min_peakiness_history_avg"].as<decltype(params_.min_peakiness_history_avg)>(
        params_.DEFAULT_MINIMUM_PEAKINESS_HISTORY_AVG );
    params_.max_2nd_der =
        node["max_2nd_derivative"].as<decltype(params_.max_2nd_der)>(
        params_.DEFAULT_MAX_2ND_DER );
    path_to_environment_ = context.resolve_path(
        node["path_to_environment"].as<decltype(path_to_environment_)>() );
    if ( path_to_environment_.back() != '/' ) {
        path_to_environment_.push_back( '/' );
    }
    path_to_mua_file_ = context.resolve_path(
        node["path_to_mua_file"].as<decltype(path_to_mua_file_)>() );
    
    update_interval_ = node["update_interval"].as<decltype(update_interval_)>(
        DEFAULT_UPDATE_INTERVAL );
    save_intermediate_values_ =
        node["save_intermediate_values"].as<decltype(save_intermediate_values_)>(
            DEFAULT_SAVE_INTERMEDIATE_VALUES );
    latency_test_ =
        node["latency_test"].as<decltype(latency_test_)>( false );
    interval_test_replay_events_ =
        node["interval_test_events"].as<decltype(interval_test_replay_events_)>(
        DEFAULT_INTERVAL_TEST_REPLAY_EVENTS );
    if ( interval_test_replay_events_ == 0 ) {
        throw ProcessingConfigureError(
            "interval_test_replay_events must be greater than zero", name());
    } else {
        LOG_IF(INFO, latency_test_) << name() << ". interval_test_replay_events = "
            << interval_test_replay_events_;
    }
    
}

void ReplayIdentifier::CreatePorts( ) {
    
    data_in_port_ = create_input_port(
        "estimates",
        LikelihoodDataType(),
        PortInPolicy( SlotRange(1) ) );
    
    data_out_port_ = create_output_port(
        "events",
        EventDataType(),
        PortOutPolicy( SlotRange(1) ) ); 
    
   history_ = create_readable_shared_state(
        "history",
        params_.initial_history,
        Permission::NONE, Permission::WRITE);
}

void ReplayIdentifier::CompleteStreamInfo() {
    
    // necessary?
    data_out_port_->streaminfo(0).datatype().Finalize();
    data_out_port_->streaminfo(0).Finalize( );
}

void ReplayIdentifier::Prepare( GlobalContext& context ) {
    
    environment_ = new Environment( path_to_environment_ );
    
    LOG(INFO) << name() << ". Environment with " << environment_->n_contents <<
        " contents and " << environment_->grid->n_elements << " grid elements "
        << "was successfully loaded.";

    reg_.reset( params_.initial_history );
    
    if ( data_in_port_->slot(0)->streaminfo().datatype().grid_size()
    != environment_->grid->n_elements ) {
        throw ProcessingPrepareError(". Loaded and incoming grid sizes do not match!",
            name());
    } else {
        LOG(DEBUG) << name() << ". Loaded and incoming grid sizes match.";
    }
    
    counter_ = new ReplayIdentificationCounter( environment_->contents_ );
    
    // load MUA array and compute MUA threshold in spikes/s
    double mua_mean = 0;
    double mua_stdev = 0;
    compute_mua_statistics( path_to_mua_file_, mua_mean, mua_stdev );
    params_.min_mua = mua_mean + params_.min_mua_std * mua_stdev;
    LOG(DEBUG) << name() << ". MUA threshold = " << params_.min_mua;
}

void ReplayIdentifier::Preprocess( ProcessingContext& context ) {
    
    if ( context.test() and save_intermediate_values_ ) {
        std::string path = context.resolve_path( "test://", "test" );
        auto prefix = path + "/" + name();
        
        streams_.clear();
        create_file( prefix, intermediate_values_names.mua );
        create_file( prefix, intermediate_values_names.mua_avg );
        create_file( prefix, intermediate_values_names.peak_gridindex );
        create_file( prefix, intermediate_values_names.peakiness );
        create_file( prefix, intermediate_values_names.peakiness_avg );
        create_file( prefix, intermediate_values_names.slope_values );
        create_file( prefix, intermediate_values_names.slope_diffs );
    }
    
    n_processed_bins_ = 0;
    first_print_done_ = false;
    params_.log_parameters( name() );
}

void ReplayIdentifier::Process( ProcessingContext& context ) {
    
    LikelihoodData* data_in = nullptr;
    EventData* data_out = nullptr;
    auto current_history = params_.initial_history;
    auto previous_history = current_history;
    bool is_candidate = false;
    bool is_replay_with_content = false;
    bool content_is_consistent = false;
    bool posterior_criteria_met = false;
    bool test_replay;
    Content content;
    
    while ( !context.terminated() ) {
        
        // get size of history
        previous_history = current_history;
        current_history = history_->get();
        if ( current_history != previous_history ) {
            check_history( current_history );
            reg_.reset( current_history );
        }  
        
        // get data
        if ( !data_in_port_->slot(0)->RetrieveData( data_in ) ) {break;}
        update_user();
        ++ n_processed_bins_;
        
        // compute mua and update relative history register
        update_history( reg_.mua, data_in->mua() ); 
               
        // compute x peak and update relative history register
        update_history( reg_.peak_grid_index, data_in->argmax() );
        
        // compute peakiness of the peak and update relative history register
        update_history( reg_.peakiness,
            compute_peakiness(data_in, reg_.peak_grid_index[0],
            params_.half_integration_window ) );
        // check peakiness
        LOG_IF(ERROR, ( reg_.peakiness[0] < 0 )) << name() <<
            ". Peakiness cannot be negative.";
        LOG_IF(ERROR, (reg_.peakiness[0] > 1)) << name() <<
            ". Peakiness value exceeds 1."; 
        
        // compute slope and update relative history register
        Linear<std::size_t, std::uint16_t>
            linear_regression( current_history,
                reg_.history_indices.data(), reg_.peak_grid_index.data() );
        update_history(reg_.slope, linear_regression.getSlope());
        
        // check if we found a replay trajectory
        is_candidate = reg_.mua_history_avg() > params_.min_mua;
        posterior_criteria_met = 
            (reg_.peakiness[0] > params_.min_peakiness) &&
            (reg_.peakiness_history_avg( nan_aware_ ) > params_.min_peakiness_history_avg) &&
            (std::abs( reg_.slope_differences() ).min() < params_.max_2nd_der);
        is_replay_with_content = is_candidate && posterior_criteria_met;

        
        // check if the replay content is consistent across the history points
        // and if so send event of related content downstream
         // always execute this block when testing for latency
        if ( is_replay_with_content or latency_test_ ) {
            // check content consistency (trajectory onset is confined within a specific content)
            content_is_consistent = true;
            content = environment_->identify_content( reg_.peak_grid_index[0] );
            for (auto it = reg_.peak_grid_index.begin()+1;
            it != reg_.peak_grid_index.end(); ++it) {
                content_is_consistent &= 
                ( content == environment_->identify_content( *it ) );
            }
            
            // produce replay events on the output port
            // produce a test replay if test mode is activated
            test_replay = latency_test_ and
                (n_processed_bins_ % interval_test_replay_events_ == 0);
            if ( (is_replay_with_content and content_is_consistent) or test_replay ) {
                data_out = data_out_port_->slot(0)->ClaimData(false);
                if ( is_replay_with_content and content_is_consistent ) {
                    data_out->set_event( "replay_" + content.name() );
                }
                if ( test_replay ) { // overwrites replays in latency test mode
                    data_out->set_event( "test_replay" );
                    ++ n_test_replays_;
                }
                data_out->set_hardware_timestamp( data_in->hardware_timestamp() );
                data_out->set_source_timestamp();
                data_out->set_serial_number( n_processed_bins_ - 1 );
                data_out_port_->slot(0)->PublishData();
                LOG_IF(DEBUG, is_replay_with_content and content_is_consistent) << name() <<
                    ". Replay trajectory of content " << content.name()
                    << " was identified";
                LOG_IF(DEBUG, test_replay) << name() <<
                    ". Test replay event was generated.";
                if ( is_replay_with_content and content_is_consistent ) {
                    counter_->add_event( content );
                }
            }
        } else if ( is_candidate ) {
            data_out = data_out_port_->slot(0)->ClaimData( false );
            data_out->set_event( "replay_unknown_content" );
            data_out->set_hardware_timestamp( data_in->hardware_timestamp() );
            data_out->set_source_timestamp();
            data_out->set_serial_number( n_processed_bins_ - 1 );
            data_out_port_->slot(0)->PublishData();
            ++ n_bins_replay_unknown_content_;
        }
        
        data_in_port_->slot(0)->ReleaseData();
        
        // save intermediate values
        if ( context.test() and save_intermediate_values_ ) {
            save_intermediate_values();
        }
    }
}

void ReplayIdentifier::Postprocess( ProcessingContext& context ) {
    
    LOG(INFO) << name() << ". There were: " << counter_->n_all_replays() <<
        " bins of replay with identified content.";
    counter_->print_content_counters( name() );
    
//    LOG_IF(INFO, counter_->all_replays_with_content()) << name() <<
//        ". All identified replays have a defined content.";
    
    LOG_IF(INFO, context.test()) << name() << ". " << n_test_replays_ <<
        " test replays were generated.";
    
    LOG(INFO) << name() << ". There were "
            << n_bins_replay_unknown_content_ << " bins with replay of unknown content.";
    
    n_test_replays_ = 0;
    n_processed_bins_ = 0;
    n_bins_replay_unknown_content_ = 0;
    
    counter_->reset( environment_->n_contents );

    streams_.clear();
}

void ReplayIdentifier::Unprepare( GlobalContext& context ) {
    
    delete environment_; environment_ = nullptr;
    delete counter_; counter_ = nullptr;
}

void ReplayIdentifier::check_history( int history ) const {
    
    LOG(UPDATE) << name() << ". History is set to " << history << " time bins.";
    
    if (history < 2) {
        LOG(ERROR) << name() << ". History must be equal or greater than 1."; 
    }
}

double ReplayIdentifier::compute_peakiness(LikelihoodData* likelihooddata,
std::size_t peak_grid_index, std::size_t half_integration_window) {
    
    // TODO: precompute win halves
    win_half1_ = std::min(half_integration_window, peak_grid_index);
    win_half2_ = std::min(half_integration_window,
        likelihooddata->grid_size() - peak_grid_index - 1);
    
    peakiness_acc_ = 0;
    for (auto g = peak_grid_index - win_half1_; g <= peak_grid_index + win_half2_; ++ g) {
        peakiness_acc_ += likelihooddata->likelihood(g);
    }
    
    return peakiness_acc_ / likelihooddata->integral_likelihood( nan_aware_ );
}

void ReplayIdentifier::save_intermediate_values() {

    streams_[intermediate_values_names.mua]->write( reinterpret_cast<const char*>(
        &reg_.mua[0] ), sizeof( decltype( reg_.mua[0] ) ) );

    auto m = reg_.mua_history_avg();
    streams_[intermediate_values_names.mua_avg]->write(
        reinterpret_cast<const char*>( &m ), sizeof( decltype( m ) ) );

    streams_[intermediate_values_names.peak_gridindex]->write(
        reinterpret_cast<const char*>( &reg_.peak_grid_index[0] ),
            sizeof( decltype( reg_.peak_grid_index[0] ) ) );

    streams_[intermediate_values_names.peakiness]->write(
        reinterpret_cast<const char*>(&reg_.peakiness[0] ),
            sizeof( decltype( reg_.peakiness[0] ) ) );
    
    auto pkns_avg = reg_.peakiness_history_avg( nan_aware_ );
    streams_[intermediate_values_names.peakiness_avg]->write(
        reinterpret_cast<const char*>(&pkns_avg ),
            sizeof( decltype( pkns_avg ) ) );

    streams_[intermediate_values_names.slope_values]->write(
        reinterpret_cast<const char*>( &reg_.slope[0]),
            sizeof( decltype( reg_.slope[0] ) ) );

    auto s = std::abs( reg_.slope_differences() ).min();
    streams_[intermediate_values_names.slope_diffs]->write(
        reinterpret_cast<const char*>(&s), sizeof( decltype( s ) ) );
}

void ReplayIdentifier::update_user() {
    
    if ( update_interval_ > 0 ) {
        LOG_IF(UPDATE, (update_interval_ > 0 ) &&
        (n_processed_bins_ % update_interval_ == 0)) << name() <<
            ". Processed " << n_processed_bins_ << " time bins.";
    }
    
    if ( !first_print_done_ ) {
            LOG(UPDATE) << name() << ". First likelihood received.";
            first_print_done_ = true;
    }
}

void ReplayIdentifier::compute_mua_statistics( std::string path_to_mua_file,
    double &mua_mean, double &mua_stdev ) {
    
    unsigned int i;
    
    if ( !path_exists( path_to_mua_file ) ) {
    throw ProcessingPrepareError(". This path doesn't exist: " + path_to_mua_file,
        name());
    }
    FILE *mua_file = NULL;
    if ( (mua_file = fopen( path_to_mua_file.c_str(), "r") ) == NULL ) {
        throw std::runtime_error("Cannot open the path to the mua file!");
    } else {
        LOG(DEBUG) << name() << ". MUA file opened from: " << path_to_mua_file;
    }
    
    uint32_t n_mua_points = 0;
    if ( get_1D_array_len( mua_file, &n_mua_points) != 0 ) {
        throw std::runtime_error("Cannot read the number of MUA points");
    } else {
        LOG(DEBUG) << name() << ". " << n_mua_points << " mua points read.";
    }
    assert( n_mua_points > 0 );

    double* mua = NULL;
    if ( (mua = get_1D_array_f64( mua_file )) == NULL ) {
        throw std::runtime_error("Cannot read the MUA file.");
    } else {
        LOG(DEBUG) << name() << ". MUA file read correctly.";
    }
    
    double sum1 = 0, sum2 = 0;
    for (i=0; i<n_mua_points; ++i) sum1 += mua[i];
    mua_mean = sum1 / n_mua_points;
    LOG(DEBUG) << name() << ". MUA mean computed:" << mua_mean;
    for (i=0; i<n_mua_points; ++i) sum2 += std::pow((mua[i] - mua_mean), 2);
    mua_stdev = std::sqrt( sum2 / n_mua_points );
    LOG(DEBUG) << name() << ". MUA standard deviation computed:" << mua_stdev;
    
    fclose(mua_file); mua_file = nullptr;
    free(mua); mua = nullptr;
}
