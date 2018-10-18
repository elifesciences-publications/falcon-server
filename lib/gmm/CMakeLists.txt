add_library(gmm component.cpp covariance.cpp mixture.cpp)
target_link_libraries( gmm gsl gslcblas )