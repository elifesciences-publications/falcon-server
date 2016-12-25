#ifndef CONNECTIONPARSER_H
#define CONNECTIONPARSER_H

#include <vector>
#include <memory>
#include <tuple>

#include "connections.hpp"

#include "utilities/general.hpp"

#define MATCH_NONE -1

typedef std::vector<std::unique_ptr<StreamConnection>> StreamConnections;

enum NodePart { PROCESSOR = 0, PORT, SLOT };

typedef std::tuple< NodePart, std::string, std::vector<int> > ConnectionPart;
typedef std::array< ConnectionPart, 3 > SingleConnectionRule;
typedef std::pair< SingleConnectionRule, SingleConnectionRule > ConnectionRule;
typedef std::vector< ConnectionRule > ConnectionRules;

ConnectionRule parseConnectionRule( std::string rulestring );
std::vector<SlotAddress> expandSingleConnectionRule( SingleConnectionRule rule );
void expandConnectionRule( ConnectionRule rule, StreamConnections& connections );

void printConnectionPart( const ConnectionPart& part );
void printSingleConnectionRule( const SingleConnectionRule& rule );
void printConnectionRule( const ConnectionRule& rule );
void printConnectionList( const StreamConnections& connections ) ;


#endif
