/* 
 * FalconProcessor: processes neural data
 * 
 * input ports:
 * data <IData> ((m - n slots)
 * other_data <IData> (m - n slots)
 * 
 * output ports:
 * data <IData> (m - n slots)
 * other_data <IData> (m - n slots)
 * 
 * exposed states:
 * state_variable1 <T1> - description
 * state_variable2 <T2> - description
 *
 * exposed methods:
 * none
 * 
 * options:
 * option1 <datatype1> - description
 * option2 <datatype2> - description
 */

#ifndef FALCON_PROCESSOR_HPP
#define	FALCON_PROCESSOR_HPP

#include "../graph/iprocessor.hpp"
#include "../data/datatype1.hpp"
#include "../data/datatype2.hpp"

class FalconProcessor : public IProcessor {
    
public:
    virtual void Configure( const YAML::Node& node, const GlobalContext& context) override;
    virtual void CreatePorts() override;
    virtual void CompleteStreamInfo() override;
    virtual void Prepare( GlobalContext& context ) override;
    virtual void Preprocess( ProcessingContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;
    virtual void Unprepare( GlobalContext& context ) override;
    
protected:
    void method1();    

protected:
    PortIn<AnyDataType>* data_in_port_;
    PortIn<AnyDataType>* other_data_in_port_;
    PortOut<AnyDataType>* data_out_port_;
    PortOut<AnyDataType>* other_data_out_port_;
    
    WritableState<T1>* state_variable1_;
    ReadableState<T2>* state_variable2_;
    
    T1 default_state_variable1_;
    T2 default_state_variable2_;

    datatype1 option1_;
    datatype2 option2_;

public:
    const datatype1 DEFAULT_OPTION1 = 6;
    static constexpr datatype DEFAULT_ARGUMENT3 = 50;
    static constexpr datatype DEFAULT_ARGUMENT4 = DEFAULT_ARGUMENT3 * 2;
};

#endif	// falconprocessor.hpp

