template <typename T>
T time2samples( double t, double rate, TruncateFlag flag ) {
    
    switch (flag) {
        case (TruncateFlag::FLOOR):
            return static_cast<T>( std::floor( t * rate ) );
        case (TruncateFlag::CEIL):
            return static_cast<T>( std::ceil( t * rate ) );
        default:
            return static_cast<T>( std::round( t * rate ) );
    }
}

template <typename T>
constexpr double samples2time( T nsamples, double rate ) {
    
    return static_cast<double>(nsamples/rate);
}
