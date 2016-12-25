
#include <string>
#include <deque>
#include <iostream>
#include <cstdlib>
//#include <std2_make_unique.hpp>
#include <regex>

#include <exception>

#include "cmdline/cmdline.h"

#include "g3log/src/g2logworker.hpp"
#include "g3log/src/g2log.hpp"
#include "g3log/src/g2loglevels.hpp"

#include "loggers/customsink.hpp"
#include "commands/commandsource.hpp"
#include "commands/commandhandler.hpp"

#include "graph/graphmanager.hpp"
#include "graph/iprocessor.hpp"

#include "context.hpp"

#include "configuration/configuration.hpp"
#include "utilities/time.hpp"

using namespace std;

int main(int argc, char** argv) {
    
    // create a parser
    cmdline::parser parser;
    
    // add specified type of variable.
    // 1st argument is long name
    // 2nd argument is short name (no short name if '\0' specified)
    // 3rd argument is description
    // 4th argument is mandatory (optional. default is false)
    // 5th argument is default value  (optional. it used when mandatory is false)
    parser.add<string>("config", 'c', "configuration file", false, "$HOME/.falcon/config.yaml" );
    parser.add("autostart", 'a', "auto start processing (needs graph)");
    parser.add("debug", 'd', "show debug messages");
    parser.add("noscreenlog", '\0', "disable logging to screen");
    parser.add("nocloudlog", '\0', "disable logging to cloud");
    parser.add("test", 't', "turn testing on by default");
    parser.footer("[graph file] ...");
    // Run parser
    // It returns only if command line arguments are valid.
    // If arguments are invalid, a parser output error msgs then exit program.
    // If help flag ('--help' or '-?') is specified, a parser output usage message then exit program.
    parser.parse_check(argc, argv);
    
    // create default configuration
    FalconConfiguration config;
    
    // load configuration file
    try {
        configuration::load_config_file<FalconConfiguration>( parser.get<std::string>("config"), config );
    } catch ( configuration::ValidationError & e ) {
        std::cout << e.what() << std::endl;
        std::cout << "Falcon terminated." << std::endl;
        return EXIT_FAILURE;
    }
    
    // apply command line arguments
    if (parser.exist("autostart")) { config.graph_autostart = true; }
    if (parser.exist("debug")) { config.debug_enabled = true; }
    if (parser.exist("noscreenlog")) { config.logging_screen_enabled = false; }
    if (parser.exist("nocloudlog")) { config.logging_cloud_enabled = false; }
    if (parser.exist("test")) { config.testing_enabled = true; }
        
    // add default URIs
    config.server_side_storage_custom["resources"] = config.server_side_storage_resources;
    config.server_side_storage_custom["graphs"] = config.server_side_storage_resources + "/graphs";
    config.server_side_storage_custom["filters"] = config.server_side_storage_resources + "/filters";
    config.server_side_storage_custom["runroot"] = config.server_side_storage_environment;
    
    GlobalContext context( config.testing_enabled, config.server_side_storage_custom );
    
    // set up loggers
    // file logger
    char *home = getenv("HOME");
    std::regex re ("(\\$HOME|~)");
    std::string logpath = std::regex_replace( config.logging_path, re, home );
    auto logger = g2::LogWorker::createWithDefaultLogger("falcon", logpath );
    
    // initialize logging before creating additional loggers
    g2::initializeLogging(logger.worker.get());
    
    // enable DEBUG logging
    g2::setLogLevel( DEBUG, config.debug_enabled );
    
    // screen logger
    if (config.logging_screen_enabled) {
        logger.worker->addSink(std2::make_unique<ScreenSink>(), &ScreenSink::ReceiveLogMessage);
        LOG(INFO) << "Enabled logging to screen.";
    }
    // cloud logger
    if (config.logging_cloud_enabled) {
        logger.worker->addSink(std2::make_unique<ZMQSink>( context.zmq(), config.logging_cloud_port ), &ZMQSink::ReceiveLogMessage);
         //wait so that any existing subscriber has a change to connect before we send out first messages
        std::this_thread::sleep_for( std::chrono::milliseconds(200) );
        LOG(INFO) << "Enabled logging to cloud on port " << std::to_string(config.logging_cloud_port);
    }
    
    LOG(INFO) << "Logging initialized. Log file saved to " << logpath;
    
    // Check clock used for internal timing
    LOG_IF(WARNING, not Clock::is_steady) <<
        "The clock used for timing is not steady.";
    
    LOG(INFO) << "Resolution of clock used for timing is " <<
        10e6 * static_cast<double>(Clock::period::num) / Clock::period::den <<
        " microseconds.";
    
    // create and start GraphManager in separate thread
    graph::GraphManager gm(context);
    gm.start();
    
    // log list of registered processors
    std::vector<std::string> processors = ProcessorFactory::instance().listEntries();
    for (auto item : processors ) {
        LOG(INFO) << "Registered processor " << item;
    }
    
    // create command sources
    // keyboard commands
    commands::KeyboardCommands kb;
    // cloud commands   
    commands::ZMQCommands zc(context.zmq(), config.network_port );
    // command line commands
    commands::CommandLineCommands cl;
    
    
    std::string graph_file = config.graph_file;
    if ( parser.rest().size()>0 ) {
        graph_file = parser.rest().back();
    }
    
    std::deque<std::string> command;
    if (graph_file.size()>0) {
        
        graph_file = context.resolve_path( graph_file, "graphs" );
        
        command.push_back("graph");
        command.push_back("buildfile");
        command.push_back(graph_file);
        
        cl.AddCommand( command );
        
        command.clear();
        
        if (config.graph_autostart) {
            command.push_back( "graph" );
            command.push_back( "start" );
            
            cl.AddCommand( command );
            
            command.clear();
        }
    }
    
    // set up Command handler
    commands::CommandHandler commandhandler(context);
    
    // add command sources to handler
    commandhandler.addSource( cl );
    commandhandler.addSource( kb );
    commandhandler.addSource( zc );
    
    LOG(INFO) << "Enabled keyboard commands.";
    LOG(INFO) << "Enabled cloud commands on port 5555.";
    
    LOG(INFO) << "Falcon started successfully.";
    // start handling commands
    commandhandler.start();
    
    
    LOG(INFO) << "Falcon shutting down normally.";
    
    return EXIT_SUCCESS;
}


