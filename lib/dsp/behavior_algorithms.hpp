#ifndef BEHAVIOR_ALGORITHMS_HPP
#define BEHAVIOR_ALGORITHMS_HPP

#include <memory>
#include <vector>
#include <limits>
#include "filter.hpp"

namespace dsp {
namespace behavior_algorithms {
  
enum SpeedSign {
    NO_SIGN = 0,
    NEGATIVE,
    POSITIVE
};
    
class SpeedCalculator {
    
public:
    SpeedCalculator(
        std::size_t n_taps_ma_position, std::size_t n_taps_ma_speed,
        double dt,
        std::size_t batch_size=DEFAULT_BATCH_SIZE,
        std::uint64_t invalid_ts_value=DEFAULT_NONEXISTENT_TIMESTAMP,
        double init_position_value=0,
        double init_speed_value=0 );
    
    std::size_t total_delay() const;
    std::size_t delay_position() const;
    std::size_t delay_speed() const;
    
//    void add_samples( std::vector<double> x_in, std::vector<std::uint64_t> timestamps_in );
//    void add_sample( double x_in, std::uint64_t ts_in );
    
    // return false if not enough sample to compute speed values
    bool compute_speed( std::vector<std::uint64_t> ts_in, std::vector<double> x_in,
        std::vector<std::uint64_t>& ts_out, std::vector<double>& speed_out );
    
    std::vector<double> smoothed_position() const;
    
    std::vector<double> unsmooothed_speed() const;
    
    std::vector<SpeedSign> speed_sign() const;
    
private:
    std::unique_ptr<dsp::filter::FirFilter> filter_position_;
    std::unique_ptr<dsp::filter::FirFilter> filter_speed_;
    
    double dt_;
    std::size_t batch_size_;
    std::size_t delay_;
    
    std::vector<std::uint64_t> timestamp_register_; 
    double last_smoothed_position_;
    std::vector<double> smoothed_position_;
    std::vector<double> unsmooothed_speed_;
    std::vector<SpeedSign> speed_sign_;
    decltype(speed_sign_) speed_sign_register_;
    
public:
    static const std::size_t DEFAULT_BATCH_SIZE = 100;
    static const std::uint64_t DEFAULT_NONEXISTENT_TIMESTAMP = 9;
};

} // behavior_algorithms
} // dsp

#endif // behavior_algorithms.hpp