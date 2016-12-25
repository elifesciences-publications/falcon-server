#ifndef GRAPHEXCEPTIONS_H
#define GRAPHEXCEPTIONS_H

#include <exception>
#include <string>

class GraphException: public std::runtime_error
{
public:
    GraphException( std::string const& error, std::string const& source="" ): std::runtime_error( source=="" ? error : source + ": " + error) {}
    virtual std::string gettype() const {return std::string("None"); }
    virtual bool isFatal() const { return true; }
};

#define GRAPHEXCEPTION( TYPE, FATAL )  class TYPE  : public GraphException {   \
        public: TYPE( std::string const& error, std::string const& source="" ): GraphException(error,source) {} \
        public: std::string gettype() const { return std::string(#TYPE); } \
        public: bool isFatal() const { return FATAL; } }

#define GRAPHERROR( TYPE )  GRAPHEXCEPTION( TYPE, true )
#define GRAPHWARNING( TYPE )  GRAPHEXCEPTION( TYPE, false )

GRAPHERROR( NoGraphError );
GRAPHERROR( InvalidGraphError );
GRAPHERROR( InvalidStateError );
GRAPHERROR( InvalidProcessorError );
GRAPHERROR( InvalidGraphCommandError );

GRAPHWARNING( BadGraphDefinition );

#endif
