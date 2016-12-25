#include <string>
#include <unistd.h>

#include "cmdline/cmdline.h"
#include "utilities/keyboard.hpp"
#include "neuralynx/nlx.hpp"
#include "utilities/string.hpp"

#include "config.hpp"

#include "datastreamer.hpp"
#include "filesource.hpp"
#include "whitenoisesource.hpp"

static constexpr const char* const DEFAULT_IP_ADDRESS = "127.0.0.1";
static const int DEFAULT_PORT = 5000;
static constexpr const double DEFAULT_RATE = 32000;
static constexpr const uint64_t DEFAULT_NPACKETS = 0;
static constexpr const char* const DEFAULT_NLX_FILE = "";
static constexpr const double DEFAULT_SAMPLING_RATE = NLX_SIGNAL_SAMPLING_FREQUENCY;

void list_all_sources( std::vector<std::unique_ptr<DataSource>> & sources ) {
    
    std::cout << std::endl;
    std::cout << "Available sources:" << std::endl;
    
    char key = 'a';
    
    for ( auto & it : sources ) {
        std::cout << key << " : " << it->string() << std::endl;
        if (key=='z') {break;} // maximum of 26 sources
        ++key;
    }
    
    std::cout << std::endl;
    
}

void print_prompt( char n ) {
    
    std::cout << "Press letter (a-" << static_cast<char>(('a' + n-1)) << ") to stream from a source. Press <Esc> to stop streaming and quit. Press <space> to list all sources." << std::endl << std::endl;
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
    parser.add<std::string>("config", 'c', "configuration file", false, "$HOME/.nlxtestbench/config.yaml" );
    parser.add<int>("autostart", 'a', "source to auto start streaming", false, -1);
    parser.add<double>("rate", 'r', "data stream rate (Hz)", false, -1);
    parser.add<int64_t>("npackets", 'n', "maximum number of packets to stream (0 means all packets)", false, -1);
    
    // Run parser
    // It returns only if command line arguments are valid.
    // If arguments are invalid, a parser output error msgs then exit program.
    // If help flag ('--help' or '-?') is specified, a parser output usage message then exit program.
    parser.parse_check(argc, argv);
    
    // create default configuration
    TestBenchConfiguration config;
    
    // load configuration file
    try {
        configuration::load_config_file<TestBenchConfiguration>( parser.get<std::string>("config"), config );
    } catch ( configuration::ValidationError & e ) {
        std::cout << e.what() << std::endl;
        std::cout << "Falcon terminated." << std::endl;
        return EXIT_FAILURE;
    }
    
    if (config.sources.size()==0) {
        std::cout << "Please define signal sources." << std::endl;
        return EXIT_FAILURE;
    }
    
    // override config with command line options
    if (parser.get<double>("rate")>0) {
        config.stream_rate = parser.get<double>("rate");
    }
    
    if (parser.get<int64_t>("npackets")>=0) {
        config.npackets = parser.get<int64_t>("npackets");
    }
    
    // auto start
    // find source with specified name
    unsigned int idx = 0;
    bool autostart = false;
    
    if (parser.get<int>("autostart")>=0) {
        idx = parser.get<int>("autostart");
        if (idx>=config.sources.size()) {
            std::cout << "Warning: cannot auto start non-existing stream " << idx << std::endl << std::endl;
            idx = 0;
        } else {
            autostart = true;
        }
    }
    
    std::cout << "NlxTestBench configuration:" << std::endl;
    std::cout << "stream rate = " << to_string_n(config.stream_rate) << " Hz " << std::endl;
    
    if ( config.npackets == 0 ) {
        std::cout << "npackets = all" << std::endl;
    } else {
        std::cout << "npackets = " << to_string_n(config.npackets) << "( " <<
            to_string_n( config.npackets / NLX_SIGNAL_SAMPLING_FREQUENCY ) << " s)" <<
            std::endl;
    }
    if (autostart) {
        std::cout << "auto start = " << static_cast<char>('a' + idx ) << std::endl;
    }
    
    // create data streaming object
    DataStreamer streamer( config.sources[idx].get(), config.stream_rate, config.ip_address, config.port, config.npackets );
    
    // print all available sources
    list_all_sources( config.sources );
    print_prompt( config.sources.size() );
    
    s_catch_sigint_signal(); // Install Ctrl-C signal handler
    nonblock(1);
    
    int hit;
    char c;
    
    if (autostart) {
        streamer.Start();
    }
    
    // process keyboard commands
    while (true) {
        
        if (streamer.terminated()) {
            streamer.Stop();
            std::cout << "Streaming done." << std::endl;
        }
        
        hit=kbhit();
        
        //check it was CTRL-C
        if (s_interrupted) {
            break;
        }
        
        if (hit!=0) {
            // get key
            c=getchar();
            
            //std::cout << "key = " << static_cast<int>(c) << std::endl;
            if (c>='a' && c<static_cast<char>('a' + config.sources.size())) {
                streamer.Stop();
                streamer.set_source( config.sources[c-'a'].get() );
                streamer.Start();
            } else if (c==27) { // Esc
                streamer.Stop();
                break;
            } else if (c==32) { // space
                list_all_sources( config.sources );
                print_prompt( config.sources.size() );
            }
            
        }
        
        usleep(100000); // 0.1 second
    }
            
    nonblock(0);
    
    return EXIT_SUCCESS;
    
}

