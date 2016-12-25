/* DummySink: takes an any data stream and eats it. Mainly used to show 
 * and test basic graph processing functionality.
 * 
 * input ports:
 * data <IData> (1 slot)
 *
 * output ports:
 * none
 *
 * exposed states:
 * tickle <bool> - logs message
 *
 * exposed methods:
 * kick - logs message
 *
 * options:
 * none
 * 
 */

#ifndef DUMMYSINK_H
#define DUMMYSINK_H

#include "../graph/iprocessor.hpp"

class DummySink : public IProcessor
{
public:
    virtual void CreatePorts() override;
    virtual void Process( ProcessingContext& context ) override;
    
    YAML::Node Kick( const YAML::Node & node );

protected:
    PortIn<AnyDataType>* data_port_;
    ReadableState<bool>* tickle_state_;
};

#endif
