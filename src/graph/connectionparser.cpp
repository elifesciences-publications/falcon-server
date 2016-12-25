#include <iostream>
#include <regex>
#include <list>

#include "connectionparser.hpp"

ConnectionRule parseConnectionRule( std::string rulestring ) {
    
    // rule
    // <specifier>:<name><id>.<specifier>:<name><id>.<specifier>:<name><id>
    // specifier is one of f (processor), p (port) or s (slot)
    // name is any character in [a-zA-Z_]
    // id is either: number or list of ranges (e.g. [1, 4-8, 10])
    
    
    ConnectionRule rule;
    SingleConnectionRule single_rules[2];
    
    std::string expr("(?:(f|p|s)\\:)?(\\w+[a-zA-Z])?((?:\\d+)|(?:\\([\\d,\\-]+\\)))?");
    std::regex re(expr);
    std::smatch match;
    
    int startid;
    int endid;
    
    int current_rule_part = 0;
    int current_connection_part = 0;
    
    // remove all spaces
    rulestring.erase( std::remove_if( rulestring.begin(), rulestring.end(), [](char x){ return std::isspace(x); } ), rulestring.end() );
    
    // split on "="
    auto rule_parts = split( rulestring, '=' );
    if (rule_parts.size()!=2 || rule_parts[0].length()==0 || rule_parts[1].length()==0)
    { throw std::runtime_error("Error parsing connection rule."); }
    
    for ( auto &rule_part : rule_parts ) {
        
        // split on "."
        auto connection_parts = split( rule_part, '.' );
        
        if (connection_parts.size()>3) { throw std::runtime_error("Error parsing connection rule."); }
        
        current_connection_part = 0;
        
        std::list<NodePart> available_specifiers { PROCESSOR, PORT, SLOT };
        NodePart specifier;
        
        for ( auto &connection_part : connection_parts ) {
            
            // match regular expression
            if ( !std::regex_match(connection_part, match, re) )
            { throw std::runtime_error("Error parsing connection rule."); }
            
            // parse part specifier
            if ( !match[1].matched ){
                //get next available specifier
                specifier = available_specifiers.front();
                available_specifiers.pop_front();
            } else {
                //check if specifier is available
                std::string piece = match[1].str();
                if (piece=="f") { specifier = PROCESSOR; }
                else if (piece=="p") { specifier = PORT; }
                else { specifier = SLOT; }
                
                auto it = std::find( available_specifiers.begin(), available_specifiers.end(), specifier );
                if (it == available_specifiers.end()) { throw std::runtime_error("Error parsing connection rule: duplicate specifier."); }
                
                available_specifiers.remove( specifier );
                
            }
            
            // parse part name
            if (!match[2].matched && specifier!=SLOT)
            { throw std::runtime_error("Error parsing connection rule."); }
            std::string name = match[2].str();
            
            // parse part identifiers
            std::vector<int> identifiers;
            
            if (!match[3].matched) {
                //match all or default
                if (specifier == SLOT)
                { identifiers.push_back( -1 ); }
                else { identifiers.push_back( MATCH_NONE ); }
            } else {
                std::string piece = match[3].str();
                if (piece[0]=='(') {
                    //match ID range vector
                    //remove brackets and spaces
                    piece.erase( std::remove_if( piece.begin(), piece.end(), [](char x){ return (x=='(' || x==')' || std::isspace(x)); } ), piece.end() );
                    
                    //split on comma
                    auto id_pieces = split( piece, ',' );
                    
                    std::regex re_range("(\\d+)(?:\\-(\\d+))?");
                    std::smatch match_range;
                    
                    //match start and end id of ranges
                    for (const auto &q : id_pieces)
                    {
                        if (std::regex_match(q, match_range, re_range)) {
                            startid = stoi( match_range[1].str() );
                            if (match_range[2].matched) { endid = stoi( match_range[2].str() ); }
                            else { endid = startid; }
                            for (auto kk = startid; kk<=endid; kk++ )
                            { identifiers.push_back( kk ); }
                        } else { throw std::runtime_error("Error parsing connection rule."); }
                    }
                } else {
                    //try to convert to int
                    try {
                        identifiers.push_back( stoi( piece ) );
                    } catch ( std::invalid_argument &e )
                    { throw std::runtime_error("Error parsing connection rule."); }
                }
            }
            
            // construct ConnectionPart and add to SingleConnectionRule
            single_rules[current_rule_part][current_connection_part] = std::make_tuple( specifier, name, identifiers );
            
            current_connection_part++;
        }
        
        // TODO: complete missing ConnectionParts of SingleConnectionRule
        // go through available specifiers
        for (auto &k : available_specifiers) {
            if ( k==PROCESSOR ) {
                throw std::runtime_error("Error parsing connection rule: no processor specified");
            } else if ( k==PORT ) {
                single_rules[current_rule_part][current_connection_part] = std::make_tuple( PORT, std::string(""), std::vector<int>(1,MATCH_NONE) );
            } else if ( k==SLOT ) {
                single_rules[current_rule_part][current_connection_part] = std::make_tuple( SLOT, std::string(""), std::vector<int>(1,-1) );
            }
            
            current_connection_part++;
        }
        
        current_rule_part++;
        
    }
        
    // construct ConnectionRule from both SingleConnectionRules
    rule = std::make_pair( single_rules[0], single_rules[1] );
    
    return rule;
    
}

std::vector<SlotAddress> expandSingleConnectionRule( SingleConnectionRule rule ) {
    
    std::array<int,3> index;
    std::array<std::string,3> names;
    int idx;
    std::array<int,3> tmp;
    std::string processor;
    std::string port;
    int slot = -1;
    
    std::vector<SlotAddress> cpoints;
    
    for ( int i = 0; i<3; i++ )
    {
        idx = std::get<0>( rule[i] );
        index[i] = idx;
        names[i] = std::get<1>( rule[i] );
    }
    
    for ( auto a : std::get<2>( rule[0] ) ) {
        for ( auto b : std::get<2>( rule[1] ) ) {
            for ( auto c : std::get<2>( rule[2] ) ) {
                tmp[0] = a;
                tmp[1] = b;
                tmp[2] = c;
                
                for ( int d=0; d<3; d++ ) {
                    if ( index[d]==0 ) { // processor
                        if (tmp[d]==MATCH_NONE)
                        { processor = names[d]; }
                        else
                        { processor = names[d] + std::to_string(tmp[d]); }
                    } else if (index[d]==1 ) { // port
                        if (tmp[d]==MATCH_NONE)
                        { port = names[d]; }
                        else
                        { port = names[d] + std::to_string(tmp[d]); }
                    } else { // slot
                        slot = tmp[d];
                    }
                }
                
                cpoints.push_back( SlotAddress( processor, port, slot ) );
                
                //std::cout << processor << "." << port << "." << slot << std::endl;
            }
        }
    }
    
    return cpoints;
    
}

void expandConnectionRule( ConnectionRule rule, StreamConnections& connections ) {
    
    // for output SingleConnectionRule
    auto out = rule.first;
    auto out_points = expandSingleConnectionRule( out );

    // for input SingleConnectionRule
    auto in = rule.second;
    auto in_points = expandSingleConnectionRule( in );

    if ( out_points.size()!=1 && out_points.size() != in_points.size() ) {
        throw std::runtime_error(
            "Invalid connection rule: number of outputs and inputs does not match.");
    }

    if (out_points.size()==1) {
        for (int i=0; i< (int)in_points.size(); i++) {
            connections.push_back( std::unique_ptr<StreamConnection>(
                new StreamConnection( out_points[0], in_points[i] ) ) );
        }
    } else {
        for (int i=0; i< (int)out_points.size(); i++) {
            connections.push_back( std::unique_ptr<StreamConnection>(
                new StreamConnection( out_points[i], in_points[i] ) ) );
        }

    }
    
}

void printConnectionPart( const ConnectionPart& part ) {
    
    std::cout << std::get<0>(part);
    std::cout << std::get<1>(part);
    
    auto v = std::get<2>(part);
    
    if (v.size()>0 && v[0]>=0) {
        std::cout << "[";
        for (auto &it : v )
        { std::cout << it << ", "; }
        std::cout << "]";
    }
    
}

void printSingleConnectionRule( const SingleConnectionRule& rule ) {
    
    for (int i=0; i<3; i++)
    {
        printConnectionPart( rule[i] );
        if (i<2) { std::cout << "."; }
    }
}

void printConnectionRule( const ConnectionRule& rule ) {
    
    printSingleConnectionRule( rule.first );
    std::cout << " = ";
    printSingleConnectionRule( rule.second );
    std::cout << std::endl;
}

void printConnectionList( const StreamConnections& connections ) {
    
    for (auto &it : connections) {
        std::cout << it->string() << std::endl;
    }
}
