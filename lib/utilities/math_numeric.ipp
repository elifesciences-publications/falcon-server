
#include <vector>

template <typename T>
std::vector<T> linspace(T min, T max, std::size_t  n) {
    
    std::vector<T> result( n );
    auto step = (max-min)/(n - 1);
    
    for (std::size_t i = 0; i < n; i++) {
        result[i] = min + i * step;
    }

    return result;
}

template <typename ForwardIterator>
double nan_sum( ForwardIterator first, ForwardIterator last ) {
    
    bool all_nan = true;
    double result = 0;   
    
    for (; first != last; ++ first) {
        if (! std::isnan( *first )) {
            result += *first;
            if ( all_nan ) { all_nan = false;}
        }
    }
    if ( all_nan ) {
        return std::nan("");
    }
    return result;
}

template <typename ForwardIterator>
double nan_mean( ForwardIterator first, ForwardIterator last, int n_elem_input ) {
    
    std::size_t n_elem;
    double acc = 0;
    std::size_t nan_counter = 0;
    
    if ( n_elem_input < 0) {
        n_elem = std::distance(first, last);
    } else {
        n_elem = n_elem_input;
    }
    
    if (n_elem < 1) {
        return 0;
    }
    
    std::size_t it_sum = 0;
    for (; first != last; ++ first) {
        if (! std::isnan( *first )) {
            acc += *first;
            ++ it_sum;
        } else {
            ++ nan_counter;
        }
    }
    if (nan_counter == n_elem) {
        return std::nan("");
    }
    
    double ret = acc / (n_elem - nan_counter);
    
    return ret;
}
