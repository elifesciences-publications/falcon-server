/*
 * Utilities for mathematical and numeric operations
 * 
 */
#ifndef MATH_NUMERIC_HPP
#define	MATH_NUMERIC_HPP

#include <cassert>
#include <limits>
#include <vector>
#include <iterator>
#include <cmath>
#include <vector>
#include <algorithm>
#include <random>
#include <memory>

// compares two double-precision floating points (or one with zero) and returns
// true if their relative error is within a certain absolute or relative tolerance
bool compare_doubles(
    double A,
    double B = 0,
    double maxAbsoluteError = std::numeric_limits<double>::epsilon(),
    double maxRelativeError = std::numeric_limits<double>::epsilon());

int next_pow2( int n );

template <typename T>
class Range {
public:
    Range( T a ) : lower_(a), upper_(a) {}
    Range( T lower, T upper ) : lower_(lower), upper_(upper) { assert( upper_>=lower_ );}
    Range( std::vector<T> limits ) {

        assert( limits.size() == 2 );
        lower_ = limits[0];
        upper_ = limits[1];
    }
    
    T lower() const { return lower_; }
    T upper() const { return upper_; }
    
    bool inrange( T value ) const { return ( value>=lower_ && value<=upper_); }
    bool inopenrange( T value ) const { return ( value>lower_ && value<upper_); }
    
    template <typename R>
    bool inrange( const Range<R> & other ) const {
        
        return (    static_cast<T>(other.lower()) >= lower_ &&
                    static_cast<T>(other.upper()) <= upper_ );
    }
    
    template <typename R>
    bool inopenrange( const Range<R> & other ) const {
        
        return (    static_cast<T>(other.lower()) > lower_ &&
                    static_cast<T>(other.upper()) < upper_ );
    }
    
    template <typename R>
    bool overlapping( const Range<R> & other ) const {
        
        return (    static_cast<T>(other.lower()) <= upper_ &&
                    static_cast<T>(other.upper()) >= lower_ );
    }
    
protected:
    T lower_;
    T upper_;
};

/* compute sum excluding NaN values */
template <typename ForwardIterator>
double nan_sum( ForwardIterator first, ForwardIterator last );

/* compute mean excluding NaN values;
 *  if the number of elements is not provided, it will be internally computed */
template <typename ForwardIterator>
double nan_mean( ForwardIterator first, ForwardIterator last, int n_elem = -1 );

/* linspace:
 * generate n elements equally spaced from min and max;
 * min and max are included in the sequence of values.
 */
template <typename T>
std::vector<T> linspace(T min, T max, std::size_t n);

template <typename T1, typename T2>
class Linear {
public:
    Linear(size_t n, T1* x, T2* y) {

        assert(n > 0);
        // calculate the averages of arrays x and y
        double xa = 0, ya = 0;
        for (size_t i = 0; i < n; i++) {
            xa += x[i];
            ya += y[i];
        }
        xa /= n;
        ya /= n;

        // calculate auxiliary sums
        double xx = 0, yy = 0, xy = 0;
        for (size_t i = 0; i < n; i++) {
            double tmpx = x[i] - xa, tmpy = y[i] - ya;
            xx += tmpx * tmpx;
            yy += tmpy * tmpy;
            xy += tmpx * tmpy;
        }

        // calculate regression line parameters

        // make sure slope is not infinite
        assert(fabs(xx) != 0);

        m_b = xy / xx;
        m_a = ya - m_b * xa;
        m_coeff = (fabs(yy) == 0) ? 1 : xy / sqrt(xx * yy);
    }
 
    double getValue(T1 x) { //! Evaluates the linear regression function at the given abscissa.
        
        return m_a + m_b * x;
    }

    double getSlope() { //! Returns the slope of the regression line
        
        return m_b;
    }

    double getIntercept() { //! Returns the intercept on the Y axis of the regression line
        
        return m_a;
    }

    double getCoefficient() { //! Returns the linear regression coefficient
        
        return m_coeff;
    }

private:
    double m_a, m_b, m_coeff;
    
};


// http://stackoverflow.com/questions/3738349/fast-algorithm-for-repeated-calculation-of-percentile
// A known disadvantage is that two heaps grow without bounds. User class should use clear() periodically.
// TODO: limit the size of the heaps, possibly with about periodic random sampling and recreating the heaps (std::make_heap)
template<class T>
class IterativePercentile {
public:
    /// Percentile has to be in range [0, 1(
    IterativePercentile( double percentile=0.5 ) : _percentile(percentile) { }
    
    std::size_t size() {
        
        return ( _lower.size() + _upper.size() );
    }

    // Adds a number in O(log(n))
    void add(const T& x) {
        
        if (_lower.empty() || x <= _lower.front()) {
            _lower.push_back(x);
            std::push_heap(_lower.begin(), _lower.end(), std::less<T>());
        } else {
            _upper.push_back(x);
            std::push_heap(_upper.begin(), _upper.end(), std::greater<T>());
        }

        unsigned size_lower = static_cast<unsigned>(
            (_lower.size() + _upper.size()) * _percentile) + 1;
        if (_lower.size() > size_lower) {
            // lower to upper
            std::pop_heap(_lower.begin(), _lower.end(), std::less<T>());
            _upper.push_back(_lower.back());
            std::push_heap(_upper.begin(), _upper.end(), std::greater<T>());
            _lower.pop_back();
        } else if (_lower.size() < size_lower) {
            // upper to lower
            std::pop_heap(_upper.begin(), _upper.end(), std::greater<T>());
            _lower.push_back(_upper.back());
            std::push_heap(_lower.begin(), _lower.end(), std::less<T>());
            _upper.pop_back();
        }            
    }

    /// Access the percentile in O(1)
    const T& get() const { return _lower.front();}

    void clear() {
        _lower.clear();
        _upper.clear();
    }

private:
    double _percentile;
    std::vector<T> _lower;
    std::vector<T> _upper;
  
};

#include "math_numeric.ipp"

#endif	// math_numeric.hpp

