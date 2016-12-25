#include "math_numeric.hpp"
#include <cmath>

int next_pow2( int n ) {
    
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;    
    return n;
}

bool compare_doubles(double A, double B, double maxAbsoluteError, double maxRelativeError) {
    
    // adapted from http://www.cygnus-software.com/papers/comparingfloats/Comparing%20floating%20point%20numbers.htm#_Toc135149453

    if ( (std::fabs(A - B) <= maxAbsoluteError) ) { return true;}

    double relativeError;

    if (std::fabs(B) > std::fabs(A)) {
        relativeError = std::fabs((A - B) / B);
    } else {
        relativeError = std::fabs((A - B) / A);
    }

    if (relativeError <= maxRelativeError) {return true;}

    return false;
}
