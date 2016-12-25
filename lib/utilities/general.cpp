#include "general.hpp"
#include "g3log/src/g2log.hpp"
#include <cmath>


void EventCounter::reset() {
    
    all_received = 0;
    target = 0;
    non_target = 0;
}

static int check_buffer_sizes(

    // can return:
    // -2: no check passed;
    // -1: loose check passed, but required strict check failed;
    //  1: loose checked passed (strict check not required))
    //  2: both checks passed (independent of the request required)
    double incoming,
    double& outgoing, // can be changed inside if no strict check is present
    bool strict_check,
    size_t & n); // number of incoming/upstream buffer-size that must be integrated to obtain one outgoing/downstream buffer-size

int check_buffer_sizes(double incoming, double& outgoing, bool strict_check, size_t & n) {
    
    if (incoming > outgoing) {
        outgoing = incoming;
        n = 1;
        return -2;
    }
       
    if ( !compare_doubles( remainder( outgoing, incoming ) ) ) { // check remainder is zero
        if ( !strict_check ) {
            n = round( outgoing / incoming );
            outgoing = n * incoming;
            return 1;
        } 
        n = 0;
        return -1; 
    }
    n = outgoing / incoming;
    return 2;
}

void check_buffer_sizes_and_log( double incoming, double& outgoing, bool strict_check, size_t & n, std::string processor_name ) {
    
    double outgoing_copy = outgoing;
    switch( check_buffer_sizes(incoming, outgoing, strict_check, n) ) {
    case -2:
        throw std::runtime_error( processor_name +
        ". Selected outgoing buffer size must be higher or equal to the incoming buffer size ("
        + std::to_string(incoming) + " ms, requested (outgoing): " + std::to_string(outgoing_copy) + " ms)" );
        break;
    case -1:
        throw std::runtime_error( processor_name +
        ". Selected outgoing buffer size must be an exact multiple of the incoming buffer size ("
        + std::to_string(incoming) + " ms, requested (outgoing): " + std::to_string(outgoing_copy) + " ms)" );
        break;
    case 1:
        LOG(UPDATE) << processor_name << ". Outgoing buffer size was adjusted to " << outgoing_copy << " ms.";
        break;
    }
}