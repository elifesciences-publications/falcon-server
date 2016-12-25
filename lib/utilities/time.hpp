#ifndef TIME_HPP
#define TIME_HPP

#include <cstdint>
#include <chrono>
#include <string>
#include <cmath>

// define clock for performance measurements
typedef std::chrono::steady_clock Clock;
typedef Clock::time_point TimePoint;

void custom_sleep_for( uint64_t microseconds );

std::string time_to_string( std::time_t t, std::string fmt = "%F %T" );

enum class TruncateFlag {ROUND=0, FLOOR, CEIL};

template <typename T>
T time2samples( double t, double rate, TruncateFlag flag = TruncateFlag::ROUND );

template <typename T>
constexpr double samples2time( T nsamples, double rate );

// returns the time difference in seconds between two timestamps
double compute_delta_ts( uint64_t t1, uint64_t t2 );

struct TimestampRegister {
    uint64_t hw;
    TimePoint source;
    
    void reset();
};

#include "time.ipp"

#endif // time.hpp
