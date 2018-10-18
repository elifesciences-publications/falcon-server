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

#include <cassert>
#include "nlx.hpp"

#include <iostream>

bool valid_nlx_vt( VideoRec* vt_record, std::uint16_t vt_id,
    ErrorNLXVT::Code& error_code, decltype(NLX_VIDEO_RESOLUTION) resolution ) {
    
    error_code = ErrorNLXVT::Code::NO_ERROR;
    if ( vt_record->swstx != VTRecSWST ) {
        error_code = ErrorNLXVT::Code::SWSTX;
        return false;
    }
    if ( vt_record->swid != vt_id ) {
        error_code = ErrorNLXVT::Code::SWID;
        return false;
    }
    if ( sizeof(VideoRec) != vt_record->swdata_size ) {
        error_code = ErrorNLXVT::Code::SWDATA_SIZE;
        return false;
    }
    if ( (vt_record->dnextracted_x < 0) or (vt_record->dnextracted_y < 0) ) {
        error_code = ErrorNLXVT::Code::NEGATIVE_COORDINATE;
        return false;
    }
    if ( vt_record->dnextracted_x >= resolution[0] or
    vt_record->dnextracted_y >= resolution[1] ) {
        error_code = ErrorNLXVT::Code::OUT_OF_RESOLUTION;
        return false;
    }
    return true;
}

NlxSignalRecord::NlxSignalRecord( unsigned int nchannels ) {
    
    set_nchannels( nchannels );
}

unsigned int NlxSignalRecord::nchannels() {
    
    return nchannels_;
}
    
void NlxSignalRecord::set_nchannels( unsigned int n ) {
    
    nchannels_ = n;
    
    nlx_nfields_ = NLX_NFIELDS( nchannels_ );
    nlx_packetbytesize_ = NLX_PACKETBYTESIZE( nchannels_ );
    nlx_field_crc_ = nlx_field_crc( nchannels_ );
    nlx_field_data_last_ = nlx_field_data_last(nchannels_);
    nlx_packetsize_ = nlx_packetsize(nchannels_);
    
    buffer_.resize( nlx_nfields_ );
    Initialize();
}
    
bool NlxSignalRecord::FromNetworkBuffer( char * buffer, size_t n, bool use_nthos_conv ) {
    
    // check size
    if (n!=nlx_packetbytesize_) {
	std::cout << ". Numbers of bytes read does not match packetsize" << std::endl;
	return false;
    }
    
    // perform ntoh conversion, copying into local buffer in the process
    char * p = (char*) buffer_.data();
    if (use_nthos_conv) {
		for ( unsigned int k=0; k<n; k+=2 ) {
			*((uint16_t*) (p+k)) = ntohs( *((uint16_t*) (buffer+k)) );
		}
	} else {
		for ( unsigned int k=0; k<n; k+=2 ) {
			*((uint16_t*) (p+k)) = *((uint16_t*) (buffer+k));
		}
	}
    
    // test if valid record (record size os OK, first 3 fields are OK, CRC checks out)
    return valid();
}
    
size_t NlxSignalRecord::ToNetworkBuffer( char * buffer, size_t n, bool use_htons_conv ) {
    
    // check size
    if (n<nlx_packetbytesize_) { return 0; }
    
    // finalize if necessary
    if (!finalized_) {Finalize();}
    
    // perform hton conversion, copying into provided buffer in the process
    char * p = (char*) buffer_.data();
    if (use_htons_conv) {
		for ( unsigned int k=0; k<nlx_packetbytesize_; k+=2 ) {
			*((uint16_t*) (buffer+k)) = htons( *((uint16_t*) (p+k)) );
		}
	} else {
		for ( unsigned int k=0; k<nlx_packetbytesize_; k+=2 ) {
			*((uint16_t*) (buffer+k)) = *((uint16_t*) (p+k));
		}
	}
    
    return nlx_packetbytesize_;
}
    
void NlxSignalRecord::Initialize() {
    
    std::fill( buffer_.begin(), buffer_.end(), 0 );
    buffer_[NLX_FIELD_STX] = NLX_STX;
    buffer_[NLX_FIELD_RAWPACKETID] = NLX_RAWPACKETID;
    buffer_[NLX_FIELD_PACKETSIZE] = nlx_packetsize(nchannels_);
    initialized_ = true;
}
    
void NlxSignalRecord::Finalize() {
    
    buffer_[nlx_field_crc_] = crc();
    finalized_ = true;
}
    
int32_t NlxSignalRecord::crc() {
    
    int32_t c = 0;
    for ( unsigned int k=0; k<nlx_nfields_-1; ++k ) {
        c ^= buffer_[k];
    }
    return c;
}

bool NlxSignalRecord::initialized() {

    return initialized_;
}

bool NlxSignalRecord::finalized() {
    
    return finalized_;
}
    
bool NlxSignalRecord::valid() {
    
    if (buffer_[NLX_FIELD_STX] != NLX_STX) {
	std::cout << ". Incorrect STX (Should be " << NLX_STX << ", but found " << buffer_[NLX_FIELD_STX] << ")." << std::endl;
        initialized_ = false;
        return false;
    }
    
    if (buffer_[NLX_FIELD_RAWPACKETID] != NLX_RAWPACKETID) {
	std::cout << ". Incorrect NLX_FIELD_RAWPACKETID (Should be " << NLX_FIELD_RAWPACKETID << ", but found " << buffer_[NLX_FIELD_RAWPACKETID] << ")." << std::endl;
        initialized_ = false;
        return false;
    }
    
    if (buffer_[NLX_FIELD_PACKETSIZE] != nlx_packetsize_) {
    	std::cout << ". Incorrect NLX_FIELD_PACKETSIZE (Should be " << nlx_packetsize_ << ", but found " << buffer_[NLX_FIELD_PACKETSIZE] << ")." << std::endl;
        initialized_ = false;
        return false;
    }
    
    if (buffer_[nlx_field_crc_] != crc()) {
	std::cout << ". Incorrect CRC (Should be " << crc() << ", but found " << buffer_[nlx_field_crc_] << ")." << std::endl;
        finalized_ = false;
        return false;
    }
    
    initialized_ = true;
    finalized_ = true;
    
    return true;
}
    
uint64_t NlxSignalRecord::timestamp() {
    
    uint64_t t;
    t = (uint32_t) buffer_[NLX_FIELD_TIMESTAMP_HIGH];
    t = (t<<32) + (uint32_t) buffer_[NLX_FIELD_TIMESTAMP_LOW];
    return t;
}
void NlxSignalRecord::set_timestamp( uint64_t t ) {
    
    buffer_[NLX_FIELD_TIMESTAMP_HIGH] = (int32_t) (t>>32);
    buffer_[NLX_FIELD_TIMESTAMP_LOW] = (int32_t) t;
    finalized_ = false;
}
void NlxSignalRecord::inc_timestamp( uint64_t delta ) {
    
    set_timestamp( timestamp() + delta );
}
void NlxSignalRecord::inc_timestamp( double delta ) {
    
    set_timestamp( timestamp() + static_cast<uint64_t>(1000000 * delta / NLX_SIGNAL_SAMPLING_FREQUENCY) );
}
    
uint32_t NlxSignalRecord::parallel_port() {
    
    return static_cast<uint32_t>( buffer_[NLX_FIELD_DIO] );
}
void NlxSignalRecord::set_parallel_port( uint32_t dio ) {
    
    buffer_[NLX_FIELD_DIO] = dio;
    finalized_ = false;
}
    
void NlxSignalRecord::data( std::vector<int32_t>& v ) {
    
    if (v.size()<nchannels_) {v.resize(nchannels_);}
    std::copy( data_begin(), data_end(), v.begin() ); 
}

std::vector<int32_t>::iterator NlxSignalRecord::data( std::vector<int32_t>::iterator it ) {
    
    return std::copy( data_begin(), data_end(), it );
}

int32_t NlxSignalRecord::sample( unsigned int index ) {
    
    return buffer_[NLX_FIELD_DATA_FIRST+index];
}
    
void NlxSignalRecord::set_data( int32_t value ) {
    
    std::fill( data_begin(), data_end(), value );
    finalized_ = false;
}

void NlxSignalRecord::set_data( std::vector<int32_t>& v ) {
    
    assert( v.size()>=nchannels_ );
    std::copy( v.begin(), v.begin()+nchannels_, data_begin() );
    finalized_ = false;
}

void NlxSignalRecord::set_data( std::vector<int32_t>::iterator it ) {
    
    std::copy( it, it+nchannels_, data_begin() );
    finalized_ = false;
}
    
void NlxSignalRecord::data( std::vector<double>& v ){
    
    if (v.size()<nchannels_) {v.resize(nchannels_);}
    data( v.begin() );
}
std::vector<double>::iterator NlxSignalRecord::data( std::vector<double>::iterator it) {
   
    auto first = data_begin();
    auto last = data_end();
    while (first!=last) {
        *it = (double) *first * NLX_AD_BIT_MICROVOLTS;
        ++it; ++first;
    }
    return it;
}

double NlxSignalRecord::sample_microvolt( unsigned int index ) {
    
    return static_cast<double>( buffer_[NLX_FIELD_DATA_FIRST+index] * NLX_AD_BIT_MICROVOLTS );
}
    
void NlxSignalRecord::set_data( double value ) {
    
    int32_t v = static_cast<int32_t> (value / NLX_AD_BIT_MICROVOLTS);
    std::fill( data_begin(), data_end(), v );
    finalized_ = false;
}

void NlxSignalRecord::set_data( std::vector<double>& v ) {
    
    assert( v.size()>=nchannels_ );
    set_data( v.begin() );
}

void NlxSignalRecord::set_data( std::vector<double>::iterator it ) {
    
    auto first = data_begin();
    for (unsigned int c=0; c<nchannels_; ++c) {
        *first = static_cast<int32_t> (*it / NLX_AD_BIT_MICROVOLTS);
        ++first; ++it;
    }
    finalized_ = false;
}

std::vector<int32_t>::iterator NlxSignalRecord::data_begin() {
    
    return buffer_.begin() + NLX_FIELD_DATA_FIRST;
}

std::vector<int32_t>::iterator NlxSignalRecord::data_end() {
    
    return buffer_.begin() + nlx_field_data_last_ + 1;
}
