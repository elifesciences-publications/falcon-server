#include <iostream>
#include <memory>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>

#include "cmdline/cmdline.h"
#include "dsp/filter.hpp"
#include "vector_operations/vector_io.hpp"
#include "utilities/time.hpp"


void test_filter_signal( std::string filename, std::string output_filename, std::unique_ptr<dsp::filter::IFilter> const & filter ) {
    
    std::vector<double> signal(0);
    
    if (filename.size()==0) { return; }
    
    // load signal data - support single channel only for now
    std::ifstream signalfile(filename, std::ios::binary);
    
    read_vector_from_binary( signalfile, signal );
    
    signalfile.close();
    
    // realize filter for single channel
    filter->realize(1);
    
    // filter data
    filter->process_channel( signal, signal );
    
    // save result
    save_vector_to_binary( output_filename, signal );
    
    filter->unrealize();
    
}

void test_filter_timing(std::unique_ptr<dsp::filter::IFilter> const & filter, unsigned int nchannels, uint64_t npoints) {
        
    // construct random distribution
    std::random_device rd;
    std::default_random_engine re(rd());
    std::normal_distribution<double> normal_dist(1.0, 2.0);
    
    // construct signal
    std::vector<std::vector<double>> multichannelsignal(0);
    for (unsigned int c = 0; c<nchannels; ++c) {
        multichannelsignal.emplace_back( npoints, 0.0 );
        for (uint64_t p = 0; p<npoints; ++p) {
            multichannelsignal[c][p] = normal_dist(re);
        }
    }
    
    // realize filter
    filter->realize( nchannels, 0.0 );
    
    auto start = Clock::now();
    
    // filter signal in place
    filter->process_by_sample( multichannelsignal, multichannelsignal );
    
    auto stop = Clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    
    std::cout << "Filtering " << npoints << " points took " << duration << " milliseconds (" << (double) duration*1000/npoints << " microseconds/point)" << std::endl;
    
    filter->unrealize();
    
}

void test_filter_impulse(std::unique_ptr<dsp::filter::IFilter> const & filter, std::string output_filename, unsigned int npoints) {
    
    if (npoints==0) { npoints = filter->order(); }
    
    // construct signal, set first(?) sample to 1
    std::vector<double> signal( npoints, 0.0 );
    signal[0] = 1.0;
    
    // realize filter
    filter->realize( 1, 0.0 );
    // filter signal
    filter->process_channel( signal, signal );
    
    filter->unrealize();
    
    // save impulse response
    save_vector_to_binary( output_filename, signal );
    
}

int main(int argc, char** argv) {
    
    // create a parser
    cmdline::parser parser;
    
    // add specified type of variable.
    // 1st argument is long name
    // 2nd argument is short name (no short name if '\0' specified)
    // 3rd argument is description
    // 4th argument is mandatory (optional. default is false)
    // 5th argument is default value  (optional. it used when mandatory is false)
    parser.add<std::string>("signal", 's', "signal to filter", false, "" );
    parser.add<std::string>("signal_output", 'o', "output file for filtered signal", false, "filtered_signal.dat" );
    parser.add("timing", 't', "perform timing of filtering operation");
    parser.add<uint64_t>("n_timing_points", 'n', "number of points for timing", false, 1000000 );
    parser.add<unsigned int>("n_timing_channels", 'c', "number of channels for timing", false, 1 );
    parser.add("impulse", 'i', "compute impulse response of filter");
    parser.add<std::string>("impulse_output", 'f', "output file for impulse response", false, "impulse_response.dat" );
    parser.add<uint32_t>("n_impulse_points", 'p', "number of points for impulse response (0 means number of samples is chosen automatically)", false, 0 );
    parser.footer("filter_definition_file");
    
    // Run parser
    // It returns only if command line arguments are valid.
    // If arguments are invalid, a parser output error msgs then exit program.
    // If help flag ('--help' or '-?') is specified, a parser output usage message then exit program.
    parser.parse_check(argc, argv);
    
    // let's open filter definition and create filter object
    if ( parser.rest().size()!=1 ) {
        std::cout << "Requires exactly one filter definition file." << std::endl;
        return EXIT_FAILURE;
    }
    
    auto filter = std::unique_ptr<dsp::filter::IFilter>( dsp::filter::construct_from_file( parser.rest()[0] ) );
    
    std::cout << "filter description: " << filter->description() << std::endl;
    std::cout << "filter order: " << filter->order() << std::endl;
    
    // 1. filter input signal
    test_filter_signal( parser.get<std::string>("signal"), parser.get<std::string>("signal_output"), filter );

    // 2. perform timing of filtering operation
    if (parser.exist("timing")) {
        unsigned int nchannels = parser.get<unsigned int>("n_timing_channels");
        uint64_t npoints = parser.get<uint64_t>("n_timing_points");
        
        test_filter_timing( filter, nchannels, npoints );
    }
    
    // 3. compute impulse response
    if (parser.exist("impulse")) {
        test_filter_impulse( filter, parser.get<std::string>("impulse_output"), parser.get<unsigned int>("n_impulse_points") );
    }
    
    return EXIT_SUCCESS;
    
}
