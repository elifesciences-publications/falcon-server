/* 
 * MultichannelDataFileStreamer: generates Multichannel Data from data stored in a NPY file.
 * The NPY file must contain a 2-dimensional numpy array; the number of elements
 * along the first dimension (rows) will be the number of channels of the Multichannel Data,
 * while the number of elements along the 2nd dimensions (columns) will be the number
 * of samples of the Multichannel Data.
 * Hardware timestamps are internally created and not loaded from file.
 * 
 * input ports:
 * none
 * 
 * output ports:
 * data <MultichannelData> ( 1 slot )
 * 
 * exposed states:
 * none
 *
 * exposed methods:
 * none
 * 
 * options:
 * filepath <string> - path to the NPY file with the data
 * sample_rate <double> - sampling frequency of the loaded data 
 * batch_size <unsigned int> - # samples in each generated Multichannel Data item
 * streaming_rate <double> - (approximate) streaming rate of the each generated Multichannel Data item 
 * initial_timestamp <uint64_t> - timestamp of the first data point
 * 
 */

#ifndef MULTICHANNELDATAFILESTREAMER_HPP
#define	MULTICHANNELDATAFILESTREAMER_HPP

#include "../data/multichanneldata.hpp"
#include "neuralynx/nlx.hpp"
#include "npyreader/npyreader.h"
#include "../graph/iprocessor.hpp"


class MultichannelDataFileStreamer : public IProcessor {
    
public:
    virtual void Configure( const YAML::Node& node, const GlobalContext& context) override;
    virtual void CreatePorts() override;
    virtual void CompleteStreamInfo() override;
    virtual void Prepare( GlobalContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;
    virtual void Unprepare( GlobalContext& context ) override;
    
protected:
    PortOut<MultiChannelDataType<double>>* data_port_;
    
    std::string filepath_;
    double sample_rate_; // depends on the loaded data
    unsigned int batch_size_;
    double streaming_rate_;
    uint64_t initial_timestamp_;
    
    uint32_t n_channels_;
    uint32_t n_samples_;
    size_t n_packets_to_dispatch_;
    double sampling_period_;
    std::vector<uint64_t> generated_hw_timestamps_;
    
    FILE* fp_;
    double** loaded_data_; 
    
public:
    static constexpr unsigned int DEFAULT_BATCH_SIZE = 10;
    static constexpr double DEFAULT_STREAMING_RATE = NLX_SIGNAL_SAMPLING_FREQUENCY/DEFAULT_BATCH_SIZE;
    const uint64_t DEFAULT_INITIAL_TIMESTAMP = 0;
    
};

#endif	// multichanneldatafilestreamer.hpp

