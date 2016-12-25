#include "time.hpp"

#include <sstream>
#include <iomanip>
#include <thread>


void custom_sleep_for( uint64_t microseconds ) {
    
    if (microseconds>1000) {
        std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
    } else {
        auto t = Clock::now() + std::chrono::microseconds(microseconds);
        while ( Clock::now() < t ) {}
    }
}

std::string time_to_string( std::time_t t, std::string fmt ) {
    
    std::stringstream s;
    s << std::put_time(std::localtime(&t), fmt.c_str());
    return s.str();
}

double compute_delta_ts( uint64_t t1, uint64_t t2 ) {
    
    int sign = 1;
    uint64_t diff;
    
    if ( t1 > t2 ) {
        diff = t1 - t2;
    } else {
        diff = t2 - t1;
        sign = -1;
    }
    
    return sign*static_cast<double>( diff )*1e-6;
}

void TimestampRegister::reset() {
    
    hw = std::numeric_limits<uint64_t>::min();
    source = TimePoint::min();
}
