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

#include "spikedata.hpp"

#include "utilities/string.hpp"
#include "vector_operations/vector_io.hpp"
#include <typeinfo>

constexpr double SpikeDataType::DEFAULT_SAMPLING_FREQUENCY;

ChannelValidityMask::ChannelValidityMask(unsigned int n_channels, ChannelDetection::Validity validity) {

    mask_.assign( n_channels, validity );
}

unsigned int ChannelValidityMask::n_channels() const {
    
    return mask_.size();
}
    
std::vector<ChannelDetection::Validity>& ChannelValidityMask::validity_mask() {
    
    return mask_;
}
    
void ChannelValidityMask::set_validity( size_t channel_index, ChannelDetection::Validity value ) {
    
    mask_[channel_index] = value;
}
    
bool ChannelValidityMask::is_channel_valid( size_t channel_index) const {
    
    return mask_[channel_index] == ChannelDetection::Validity::VALID;
}
    
bool ChannelValidityMask::all_channels_valid() const {
    
    for (auto m: mask_) {
        if (m != ChannelDetection::Validity::VALID) {
            return false;
        }
    }
    return true;
}

void ChannelValidityMask::reset(ChannelDetection::Validity validity_value) {
    
    mask_.assign( this->n_channels(), validity_value );
}

void SpikeData::Initialize ( unsigned int nchannels, size_t max_nspikes, double sample_rate) {
    
    if (nchannels==0) {
        throw std::runtime_error("SpikeData::Initializer - number of channels needs to be larger than 0.");
    }
    if (max_nspikes==0) {
        throw std::runtime_error("SpikeData::Initializer - max_nspikes needs to be larger than 0.");
    }
    if (sample_rate<=0) {
        throw std::runtime_error("SpikeData::Initializer - sample rate needs to be larger than 0.");
    }
    n_channels_ = nchannels;
    n_detected_spikes_ = 0;
    sample_rate_ = sample_rate;
    
    // overestimates the maximum number of spike features in a buffer and
    // reserve enough space so that no memory allocation will take place during run
    amplitudes_.reserve( nchannels * max_nspikes ); 
    hw_ts_detected_spikes_.reserve( max_nspikes );
    
    validity_mask_ = ChannelValidityMask( nchannels );
}

unsigned int SpikeData::n_channels() const {

    return n_channels_;
}

double SpikeData::sample_rate() const {
    
    return sample_rate_;
}

void SpikeData::add_spike(const std::vector<double>& amplitudes, uint64_t hw_timestamp) {
    
    assert ( amplitudes.size() == n_channels_ );
    for (unsigned int i=0; i < n_channels_; ++i ) {
        amplitudes_.push_back(amplitudes[i]);
    }
    ++ n_detected_spikes_;
    hw_ts_detected_spikes_.push_back(hw_timestamp);
}

void SpikeData::add_spike(double* amplitudes, uint64_t hw_timestamp) {
    
    for (unsigned int i=0; i < n_channels_; ++i ) {
        amplitudes_.push_back(amplitudes[i]);
    }
    ++ n_detected_spikes_;
    hw_ts_detected_spikes_.push_back(hw_timestamp);
}

unsigned int SpikeData::n_detected_spikes() const {

    return n_detected_spikes_;
}
     
std::vector<double>&  SpikeData::amplitudes() {
    
    assert (n_detected_spikes_ == amplitudes_.size() / n_channels_);
    return amplitudes_;
}
    
void SpikeData::ClearData() {
       
    n_detected_spikes_ = 0;
    amplitudes_.clear();
    hw_ts_detected_spikes_.clear();
    validity_mask_.reset();
}

ChannelValidityMask& SpikeData::validity_mask() {
    
    return validity_mask_;
}

const std::vector<uint64_t>& SpikeData::ts_detected_spikes() const {
    
    return hw_ts_detected_spikes_;
}

const uint64_t SpikeData::ts_detected_spikes( int index ) const {
    
    assert( n_detected_spikes_ == ts_detected_spikes().size() );
    assert( index < ( static_cast<int>(n_detected_spikes_) ) );
    return hw_ts_detected_spikes_[index];
}

std::vector<double>::const_iterator SpikeData::spike_amplitudes( std::size_t spike_index ) const {
    
    std::vector<double>::const_iterator it = amplitudes_.begin();
    it += spike_index * n_channels_;
    return it;
}

void SpikeData::SerializeBinary( std::ostream& stream, Serialization::Format format ) const {

    IData::SerializeBinary( stream, format );
    
    if ( format==Serialization::Format::FULL ) {
        
        int n_spikes_to_fill_buffer = MAX_N_SPIKES_IN_BUFFER - n_detected_spikes_;
        assert ( n_spikes_to_fill_buffer >= 0 );
        
        stream.write( reinterpret_cast<const char*>( &n_detected_spikes_ ) ,
            sizeof(decltype(n_detected_spikes_)) );
        stream.write( reinterpret_cast<const char*>( hw_ts_detected_spikes_.data() ), 
            n_detected_spikes_ * sizeof(decltype(hw_ts_detected_spikes_[0])) );
        stream.write( reinterpret_cast<const char*>( zero_timestamps.data() ), 
            n_spikes_to_fill_buffer * sizeof(decltype(zero_timestamps[0])) );
        stream.write( reinterpret_cast<const char*>( amplitudes_.data() ),
            n_detected_spikes_ * n_channels_ * sizeof(decltype(amplitudes_[0])) );
        stream.write( reinterpret_cast<const char*>( zero_amplitudes.data() ),
            n_spikes_to_fill_buffer * n_channels_ * sizeof(decltype(zero_amplitudes[0])) );
    }
    
    if ( format==Serialization::Format::COMPACT ) {
        
        for (decltype(n_detected_spikes_) sp=0; sp < n_detected_spikes_; ++ sp) {
            stream.write( reinterpret_cast<const char*>(&hw_ts_detected_spikes_[sp]),
                sizeof(decltype(hw_ts_detected_spikes_[0])) );
            stream.write( reinterpret_cast<const char*>(&amplitudes_[sp * n_channels_]),
                sizeof(decltype(amplitudes_[0])) * n_channels_ );
        }
    }
}

void SpikeData::SerializeYAML( YAML::Node & node, Serialization::Format format ) const {

    IData::SerializeYAML( node, format );
    
    if ( format==Serialization::Format::FULL || format==Serialization::Format::COMPACT ) {
        node[N_CHANNELS_S] = static_cast<unsigned int>(n_channels_);  // TODO: move to preamble
        node[N_DETECTED_SPIKES_S] = static_cast<unsigned int>(n_detected_spikes_);
        if (n_detected_spikes_ > 0) {
            node[TS_DETECTED_SPIKES_S] = hw_ts_detected_spikes_;
            node[SPIKE_AMPLITUDES_S] = amplitudes_;
        }
    }
}

void SpikeData::YAMLDescription( YAML::Node & node, Serialization::Format format ) const {

    IData::YAMLDescription( node, format );
    
    if ( format==Serialization::Format::FULL ) {
        node.push_back( N_DETECTED_SPIKES_S + " " +
            get_type_string<decltype(n_detected_spikes_)>() + " (1)" );
        node.push_back( TS_DETECTED_SPIKES_S + " " +
            get_type_string<uint64_t>() +
            " (" + std::to_string(MAX_N_SPIKES_IN_BUFFER) + ")" );
        node.push_back( SPIKE_AMPLITUDES_S + " " + get_type_string<double>() +
            " (" + std::to_string(MAX_N_SPIKES_IN_BUFFER) + ","
            + std::to_string(n_channels_) + ")" );
    }
    
    if ( format == Serialization::Format::COMPACT ) {
        node.push_back( TS_DETECTED_SPIKES_S + " " +
            get_type_string<uint64_t>() + " (1)" );
        node.push_back( SPIKE_AMPLITUDES_S + " " + get_type_string<double>() + 
            " (" + std::to_string(n_channels_) + ")" );
    }
}

double SpikeDataType::buffer_size() const {
    
    return buffer_size_ms_;
}

double SpikeDataType::sample_rate() const {
    
    return sample_rate_;
}

ChannelRange SpikeDataType::channel_range() const {
    
    return channel_range_;
}

unsigned int SpikeDataType::n_channels() const {
    
    return n_channels_;
}

bool SpikeDataType::CheckCompatibility( const SpikeDataType& upstream ) const {
    
    return channel_range_.inrange( upstream.channel_range() ); 
}

void SpikeDataType::Finalize( unsigned int nchannels, double sample_rate ) {
        
    if (  nchannels==0 || !channel_range_.inrange(nchannels) ) {
        throw std::runtime_error( "Number of channels is out of range.");
    }
    n_channels_ = nchannels;
    sample_rate_ = sample_rate;
    AnyDataType::Finalize();
}

void SpikeDataType::Finalize( SpikeDataType& upstream ) {

    Finalize( upstream.n_channels(), upstream.sample_rate() );
}

void SpikeDataType::InitializeData( SpikeData& item ) const  {
    
    unsigned int max_nspikes = round (buffer_size_ms_ * sample_rate_ / 1000) / 2;
    item.Initialize( n_channels_, max_nspikes, sample_rate_ );
}
