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

#include "behavior_algorithms.hpp"

using namespace dsp::behavior_algorithms;

SpeedCalculator::SpeedCalculator( std::size_t n_taps_ma_position,
std::size_t n_taps_ma_speed, double dt, std::size_t batch_size,
std::uint64_t invalid_ts_value, double init_position_value, double init_speed_value ) {
    
    std::vector<double> coefficients_ma;
    
    coefficients_ma.assign( n_taps_ma_position, 1/static_cast<double>( n_taps_ma_position ) );
    
    filter_position_.reset( new filter::FirFilter( coefficients_ma, "Moving Average" ) );
    filter_position_->realize( 1, init_position_value );
    
    coefficients_ma.assign( n_taps_ma_speed, 1/static_cast<double>( n_taps_ma_speed ) );

    filter_speed_.reset( new filter::FirFilter( coefficients_ma, "Moving Average") );
    filter_speed_->realize( 1, init_speed_value );
    
    delay_ = filter_position_->group_delay() + filter_speed_->group_delay();
    
    if ( batch_size < delay_ +1 ) {
        throw std::runtime_error("Batch size is too small.");
    }
    batch_size_ = batch_size;
    smoothed_position_.assign( batch_size, init_position_value );
    last_smoothed_position_ = init_position_value;
    unsmooothed_speed_.assign( batch_size, init_speed_value );
    speed_sign_.assign( batch_size, SpeedSign::NO_SIGN );
    speed_sign_register_.assign( filter_speed_->group_delay(), SpeedSign::NO_SIGN );
    
    
    if ( dt <= 0 ) {
        throw std::runtime_error("dt must be a positive number.");
    }
    dt_ = dt;
    
    timestamp_register_.assign( delay_, invalid_ts_value );
    
}

std::size_t SpeedCalculator::total_delay() const {
    
    return delay_;
}

std::size_t SpeedCalculator::delay_position() const {

    return filter_position_->group_delay();
}

std::size_t SpeedCalculator::delay_speed() const {

    return filter_speed_->group_delay();
}

bool SpeedCalculator::compute_speed(    std::vector<std::uint64_t> ts_in, 
                                        std::vector<double> x_in,
                                        std::vector<std::uint64_t>& ts_out,
                                        std::vector<double>& speed_out ) {
    
    if ( x_in.size() >= batch_size_ ) {
        
        assert( ts_in.size() == x_in.size() );
        unsigned int i;
        double linear_velocity;
//        bool speed_sign;
        
        // create the output timestamp as being shifted backwards in time
        // 1) copy the first samples of the register
        for ( i=0; i<delay_; ++i ) {
            ts_out[i] = timestamp_register_[i];
        }
        // 2) copy the remaining timestamps from the incoming ones
        for ( i=delay_; i<batch_size_; ++i) {
            ts_out[i] = ts_in[i-delay_];
        }
        
        // store the last timestamps in the register
        for ( unsigned int i=0; i < delay_; ++i ) {
            timestamp_register_[i] = ts_in[batch_size_-delay_+i];
        }
        
        // smooth linear position
        filter_position_->process_by_channel( batch_size_, x_in, smoothed_position_ );

        // compute speed as derivative with fixed dt
        unsmooothed_speed_[0] = (smoothed_position_[0] - last_smoothed_position_) / dt_;     
        for ( i=1; i<batch_size_; ++i ) {
            linear_velocity = (smoothed_position_[i] - smoothed_position_[i-1]) / dt_;
            unsmooothed_speed_[i] = std::abs( linear_velocity );
            speed_sign_[i] = SpeedSign::NO_SIGN;
            if ( linear_velocity > 0 ) {
                speed_sign_[i] = SpeedSign::POSITIVE;
            } else if ( linear_velocity < 0 ) {
                speed_sign_[i] = SpeedSign::NEGATIVE;
            }
        }
        last_smoothed_position_ = smoothed_position_[batch_size_-1];//smoothed_position_.back();
        
        // compute smoothed speed
        speed_out.resize( batch_size_ );
        filter_speed_->process_by_channel( batch_size_, unsmooothed_speed_, speed_out );
        
        // add delay to speed sign ...
        auto it = speed_sign_.rbegin();
        auto boundary = speed_sign_.rbegin()+filter_speed_->group_delay();
        auto it_reg = speed_sign_register_.rend();
        // copy samples from last to new in the register first ... 
        for ( it=speed_sign_.rbegin(); it != boundary; ++it ) {
            *it_reg = *it;
            ++ it_reg;
        }
        /// ... and then shift each value forward (increase delay) 
        for ( it=boundary; it != speed_sign_.rend(); ++it ) {
            *( it - filter_speed_->group_delay() ) = *it;
        }
        return true;
    }
    
    return false;
}

std::vector<double> SpeedCalculator::smoothed_position() const {
    
    return smoothed_position_;
}

std::vector<double> SpeedCalculator::unsmooothed_speed() const {
    
    return unsmooothed_speed_;
}

std::vector<SpeedSign> SpeedCalculator::speed_sign() const {
    
    return speed_sign_;
}