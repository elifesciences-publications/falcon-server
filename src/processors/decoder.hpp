// ---------------------------------------------------------------------
// This file is part of falcon-server.
// 
// Copyright (C) 2015, 2016, 2017 Neuro-Electronics Research Flanders
// 
// Falcon-server is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Falcon-server is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with falcon-server. If not, see <http://www.gnu.org/licenses/>.
// ---------------------------------------------------------------------

/* 
 * Decoder:
 * This processor transforms spike data into a probability function (likelihood)
 * over the stimulus space defined via a grid. It applies a clusterless
 * neural decoding algorithm with a compressed model. The user can configure
 * the number of amplitudes and the bin-size to be used for decoding. 
 * 
 * input ports:
 * spikes <SpikeData> (1 slot)
 *
 * output ports:
 * estimates <LikelihoodData> (1 slot)
 *
 * exposed states:
 * encoding_model <intptr_t> - pointer to the data structure containing the model
 * together with some precomputations of the decoding elements
 *
 * exposed methods:
 * none
 *
 * options:
 * bin_size - time window for computing the likelihood [ms]
 * n_channels <unsigned int> - # amplitudes used to apply the clusterless decoding algorithm
 * use_offline_model <bool> - if True a model must be provided, if False model
 * will be read from a connected encoder
 * path_to_offline_model <string> - path to the model created offline, ignored if using
 * online model
 * path_to_grid <string> - path to the environment grid 
 * strict_time_bin_check <bool> - whether the buffer size will be strictly or loosely checked
 * 
 */

#ifndef DECODER_HPP
#define	DECODER_HPP

#include "../graph/iprocessor.hpp"
#include "../data/spikedata.hpp"
#include "encoding/encodingmodel.hpp"
#include "../data/likelihooddata.hpp"


class Decoder : public IProcessor {
    
public:
    virtual void Configure( const YAML::Node & node, const GlobalContext& context) override;
    virtual void CreatePorts( ) override;
    virtual void CompleteStreamInfo() override;
    virtual void Prepare( GlobalContext& context) override;
    virtual void Preprocess(ProcessingContext& context) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;
    virtual void Unprepare( GlobalContext& context ) override;

protected:
    inline EncodingModel* get_model() {
        // must return naked pointer because of casting and I don't know if the
        // casting will work fine with a smart pointer
        if ( use_offline_model_ ) {
            return offline_model_;
        }
        return reinterpret_cast<EncodingModel*> (encoding_model_->get() );
    }
    void evaluate_test_amplitudes( EncodingModel* model, unsigned int n_spikes );
    
protected:
    PortIn<SpikeDataType>* data_in_port_;
    PortOut<LikelihoodDataType>* data_out_port_;
    ReadableState<std::intptr_t>* encoding_model_;
    
    double time_bin_ms_; // decoding bin size
    bool strict_time_bin_check_; // if true, time_bin will be checked according to strict rules, otherwise it will be adjusted if not in compliance
    double spike_buffer_size_; // buffersize of the incoming spikedata stream
    size_t n_target_spikebuffers_; // # spike buffers that must be read to compute the likelihood of one bin
    unsigned int n_features_;
    
    std::string path_to_offline_model_;
    std::string path_to_grid_;
    std::size_t n_grid_points_;
    bool use_offline_model_;
    EncodingModel* offline_model_; // naked because get_model uses it
    
    std::vector<double> pax_;
    std::vector<uint16_t> dimensions_;
    
    std::string path_to_test_spikes_;
    bool use_test_amplitudes_;
    FILE* fp_test_amplitudes_;
    double* loaded_spike_amplitudes_;
    double* test_spike_amplitudes_ptr_;
    std::size_t n_used_spikes_;
    uint32_t n_test_spikes_;
    
public:
    const decltype(n_features_) MAX_N_AMPLITUDES = 8;
    const decltype(time_bin_ms_) DEFAULT_BIN_SIZE = 10; // in ms
    const decltype(n_features_) MAX_N_SPIKES_PER_SPIKE_BUFFER = 200;
    const bool DEFAULT_STRICT_TIME_BIN_CHECK = true;
    const decltype(path_to_test_spikes_) NULL_PATH = "none";
};


#endif	// decoder.hpp

