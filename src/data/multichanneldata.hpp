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

#ifndef MULTICHANNELDATA_H
#define MULTICHANNELDATA_H

#include "idata.hpp"
#include <vector>
#include <cmath>

#include "utilities/string.hpp"
#include "utilities/general.hpp"
#include "utilities/iterators.hpp"
#include "g3log/src/g2log.hpp"

typedef Range<size_t> SampleRange;

template <typename T>
class MultiChannelData : public IData {
public:
    
    typedef stride_iter<T*> channel_iterator;
    typedef T* sample_iterator;
    
    MultiChannelData() {}
    
    MultiChannelData( size_t nchannels, size_t nsamples, double sample_rate ) {
        
        Initialize( nchannels, nchannels, sample_rate );
    }

    virtual void ClearData() override {
        
        std::fill( data_.begin(), data_.end(), 0);
        std::fill( timestamps_.begin(), timestamps_.end(), 0);
    }

    void Initialize( size_t nchannels, size_t nsamples, double sample_rate ) { 

        if (nchannels==0 || nsamples==0) {
            throw std::runtime_error("MultiChannelData::Initialize - number of channels/samples needs to be larger than 0.");
        }

        if (sample_rate<=0) {
            throw std::runtime_error("MultiChannelData::Initialize - sample rate needs to be larger than 0.");
        }

        nchannels_ = nchannels;
        nsamples_ = nsamples;
        sample_rate_ = sample_rate;

        data_.resize( nchannels_*nsamples_ );

        timestamps_.resize( nsamples_ );
    }
	
    size_t nchannels() const { return nchannels_; }
    size_t nsamples() const { return nsamples_; }
    double sample_rate() const { return sample_rate_; }
       
    uint64_t sample_timestamp( size_t sample = 0 ) const { return timestamps_[sample]; }
    std::vector<uint64_t>& sample_timestamps() { return timestamps_; }
    
    void set_sample_timestamp( size_t sample, uint64_t t ) {
        
        if ( sample >= nsamples_ ) {
            std::runtime_error(". Requested sample cannot be accessed");
        } else {
            timestamps_[sample] = t;
        }
    }
    
    void set_sample_timestamps( std::vector<uint64_t> &t ) {

        assert( t.size() == nsamples_ );
        timestamps_ = t;
    }
    
    void set_data_channel( size_t channel, std::vector<T>& data ) {

        assert( data.size() == nsamples_ );
        T* ptr = data_.data();
        for (size_t k=0; k<nsamples_; ++k ) {
            (*ptr)=data[k];
            ptr += nchannels_;
        }
    }
    
    void set_data_sample( size_t sample, std::vector<T>& data ) {

        assert( data.size() == nchannels_ );
        std::copy( data.begin(), data.end(), begin_sample( sample ) );
    }
    
    void set_data_sample( size_t sample, size_t channel, T data ) {
        
        if ( sample >= nsamples_ ) {
            std::runtime_error("Requested sample cannot be accessed.");
        } else {
            data_[flat_index(sample,channel)] = data;
        }
    }
    
    std::vector<T>& data() { return data_; }
    
    const T& data_sample( size_t sample, size_t channel = 0 ) const { return data_[flat_index(sample,channel)]; }
	
    T& operator()( size_t index ) { return data_[index]; }
    const T& operator()( size_t index ) const { return data_[index]; }
    
    T& operator()( size_t sample, size_t channel = 0 ) { return data_[flat_index(sample,channel)]; }
    const T& operator()( size_t sample, size_t channel = 0 ) const { return data_[flat_index(sample,channel)]; }
	
    // iterators
    T* begin_sample( size_t sample ) { return &data_[flat_index(sample)]; }
    T* end_sample( size_t sample ) { return begin_sample(sample) + nchannels_; }
    const T* begin_sample( size_t sample ) const { return &data_[flat_index(sample)]; }
    const T* end_sample( size_t sample ) const { return begin_sample(sample) + nchannels_; }
    
    stride_iter<T*> begin_channel( size_t channel ) { return stride_iter<T*>(&data[channel],nchannels_); }
    stride_iter<T*> end_channel( size_t channel ) { return begin_channel(channel) + nsamples_; }
    
    virtual void SerializeBinary( std::ostream& stream, Serialization::Format format = Serialization::Format::FULL ) const override {
        
        IData::SerializeBinary( stream, format );
        if (format==Serialization::Format::FULL) {
            stream.write( reinterpret_cast<const char*>( timestamps_.data() ), timestamps_.size() * sizeof(uint64_t) );
            stream.write( reinterpret_cast<const char*>( data_.data() ), data_.size() * sizeof(T) );
        }
        
        if (format==Serialization::Format::COMPACT) {
            
            for (size_t k=0; k<nsamples_; ++k) {
                stream.write( reinterpret_cast<const char*>(&timestamps_[k]), sizeof(uint64_t) );
                stream.write( reinterpret_cast<const char*>(&data_[flat_index(k)]), sizeof(T)*nchannels_ );
            }
        }
    }
    
    virtual void SerializeYAML( YAML::Node & node, Serialization::Format format = Serialization::Format::FULL ) const override {
            
        IData::SerializeYAML( node, format );
        if (format==Serialization::Format::FULL || format==Serialization::Format::COMPACT) {
            node["timestamps"] = timestamps_;
            // TODO: write samples individually to list of lists, instead of a single flat list
            node["signal"] = data_;
        }
    }
    
    virtual void YAMLDescription( YAML::Node & node, Serialization::Format format = Serialization::Format::FULL ) const override {
        
        IData::YAMLDescription( node, format );
        if (format==Serialization::Format::FULL) {
            node.push_back( "timestamps uint64 (" + std::to_string(nsamples_) + ")" );
            node.push_back( "signal " + get_type_string<T>() + " (" + std::to_string(nchannels_) + "," + std::to_string(nsamples_) + ")" );
        }
        
        if (format==Serialization::Format::COMPACT) {
            node.push_back( "timestamps uint64 (1)" );
            node.push_back( "signal " + get_type_string<T>() + " (" + std::to_string(nchannels_) + ")" );
        }
    }
    
    T sum_abs_sample( size_t sample ) const {
        return std::accumulate( begin_sample(sample), end_sample(sample), 0, [](T a, T b) { return a + std::abs(b); } );
    }
    
    T sum_sample( size_t sample ) const {
        return std::accumulate( begin_sample(sample), end_sample(sample), 0.0 );
    }
    
    T mean_abs_sample( size_t sample ) const {
        return sum_abs_sample(sample) / nchannels_;
    }
    
    T mean_sample( size_t sample ) const {
        return sum_sample(sample) / nchannels_;
    }

protected:
    inline size_t flat_index( size_t sample, size_t channel ) const { return channel + sample*nchannels_; }
    inline size_t flat_index( size_t sample ) const { return sample*nchannels_; }
    
protected:
    size_t nchannels_;
    size_t nsamples_;
    double sample_rate_;
    std::vector<T> data_;
    std::vector<uint64_t> timestamps_;
};

template<typename T>
class MultiChannelDataType : public AnyDataType {

ASSOCIATED_DATACLASS(MultiChannelData<T>)

public:
	
    MultiChannelDataType( size_t nchannels = 1 ) :
    AnyDataType(false),
    channel_range_( nchannels ),
    sample_range_( 1, std::numeric_limits<uint32_t>::max() ),
    nchannels_(0), nsamples_(0)
    {}
	
    MultiChannelDataType( ChannelRange channel_range,
    SampleRange sample_range = SampleRange( 1, std::numeric_limits<uint32_t>::max()) ) :
    AnyDataType(false), channel_range_(channel_range),
    sample_range_(sample_range), nchannels_(0), nsamples_(0)
    {}
	
    size_t nchannels() const { return nchannels_; }
    size_t nsamples() const { return nsamples_; }
    double sample_rate() const { return sample_rate_; }
	
    const ChannelRange& channel_range() const { return channel_range_; }
    const SampleRange& sample_range() const { return sample_range_; }
    
    virtual void Finalize( size_t nsamples, size_t nchannels, double sample_rate) {
        
        if (nsamples==0 || !sample_range_.inrange(nsamples) || nchannels==0 || !channel_range_.inrange(nchannels) ) {
            throw std::runtime_error( "Number of channels and/or samples out of range.");
        }
        nchannels_ = nchannels;
        nsamples_ = nsamples;
        sample_rate_ = sample_rate;
        AnyDataType::Finalize();
    }

    virtual void Finalize( MultiChannelDataType& other ) {
        
        Finalize( other.nsamples(), other.nchannels(), other.sample_rate() );
    }

    bool CheckCompatibility( const MultiChannelDataType<T>& upstream ) const {      
    
        auto check1 = channel_range_.overlapping( upstream.channel_range() );
        auto check2 = sample_range_.overlapping( upstream.sample_range() ) ;
        if (check1 && check2 ) {
            LOG(DEBUG) << "MultichannelData types are compatible (ranges do overlap)." << std::endl;
        } else {
            if (!check1) {
                LOG(ERROR) << "MultichannelData types have non-overlapping channel ranges.";
            }
            if (!check2) {
                LOG(ERROR) << "MultichannelData types have non-overlapping sample ranges.";
            }
        }
        return ( check1 && check2 );
    }
	
    virtual void InitializeData( MultiChannelData<T>& item ) const {
    
        item.Initialize( nchannels_, nsamples_, sample_rate_ );
    }
    
    virtual std::string name() const { return "multichannel"; }

protected:
    ChannelRange channel_range_;
    SampleRange sample_range_;
	
    size_t nchannels_;
    size_t nsamples_;
    double sample_rate_;
};

#endif // multichanneldata.hpp
