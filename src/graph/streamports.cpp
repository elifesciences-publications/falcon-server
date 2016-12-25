#include "streamports.hpp"

int IdentifyNextSlot( int slot_request, int connected_slot_number, bool allow_multi_connect, const PortPolicy& policy ) {
    
    if ( slot_request >= policy.max_slot_number() ) { return -1; }
    
    // auto select first available slot
    if ( slot_request < 0 ) {
        if (allow_multi_connect) {
            return connected_slot_number % policy.max_slot_number();
        } else if ( connected_slot_number < policy.max_slot_number() ) {
            return connected_slot_number;
        } else {
            return -1;
        }
    }

    // requested already connected slot
    if ( slot_request < connected_slot_number ) {
        if ( allow_multi_connect ) {
            return slot_request;
        } else {
            return -1;
        }
    }
    
    if ( slot_request < policy.min_slot_number() ) { return slot_request; }
    
    if ( slot_request >= policy.min_slot_number() && slot_request==connected_slot_number ) {
        return slot_request;
    }
    
    return -1;
    
}
