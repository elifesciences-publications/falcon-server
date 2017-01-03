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

// to handle incoming records (e.g. nlxreader)
// construct a NlxRecord object with required number of channels
// read packet into char buffer and call NlxRecord::FromNetworkBuffer
// if valid buffer (i.e. record size is OK, first 3 fields are OK, CRC checks out)
// grab data, timestamp, parallel port value with member methods

// to create new record
// construct a NlxRecord object with required number of channels
// optionally Initialize() (will be done during construction)
// set timestamp, parallel port values and data using member methods
// Finalize() (will compute CRC)
// copy packet into external buffer to be sent over network using ToNetworkBuffer


#ifndef NLX_H
#define NLX_H

#include <cstdint>
#include <vector>
#include <array>
#include <cmath>
#include <netinet/in.h>

// a digilynx raw packet has the following layout
//  int32            | start of transmission identifier == 2048
//  int32            | packet type identifier == 1
//  int32            | packet size == #channels + #extra
// uint32            | timestamp (high order bits)
// uint32            | timestamp (low order bits)
//  int32            | status (reserved)
// uint32            | parallel input port value
//  int32[10]        | extra values (reserved)
//  int32[nchannels] | data values (nchannels == number of channels in acquisition system)
//  int32            | CRC value for record

// some values are needed at compile time
constexpr uint16_t NLX_NFIELDS_EXTRA = 10;
constexpr uint16_t NLX_FIELDBYTESIZE = 4;
constexpr uint16_t NLX_NFIELDS(uint16_t c) { return (8 + NLX_NFIELDS_EXTRA + (c)); }
constexpr uint16_t NLX_PACKETBYTESIZE(uint16_t c) { return (NLX_FIELDBYTESIZE * NLX_NFIELDS(c)); }
constexpr uint16_t NLX_FIELD_STX = 0;
constexpr uint16_t NLX_FIELD_RAWPACKETID = 1;
constexpr uint16_t NLX_FIELD_PACKETSIZE = 2;
constexpr uint16_t NLX_FIELD_TIMESTAMP_HIGH = 3;
constexpr uint16_t NLX_FIELD_TIMESTAMP_LOW = 4;
constexpr uint16_t NLX_FIELD_DIO = 6;
constexpr uint16_t NLX_FIELD_EXTRA_FIRST = 7;
constexpr uint16_t NLX_FIELD_DATA_FIRST = NLX_FIELD_EXTRA_FIRST + NLX_NFIELDS_EXTRA;

constexpr double NLX_AD_BIT_MICROVOLTS = 0.015624999960550667;
constexpr int32_t NLX_STX = 2048;
constexpr int32_t NLX_RAWPACKETID = 1;
constexpr uint16_t NLX_DEFAULT_NCHANNELS = 128;
constexpr double NLX_SIGNAL_SAMPLING_FREQUENCY = 32000;
constexpr unsigned int NLX_DEFAULT_BUFFERSIZE = NLX_PACKETBYTESIZE(NLX_DEFAULT_NCHANNELS);

const double NLX_VIDEO_SAMPLING_FREQUENCY = 25;
const int VTRecNumTransitionBitfields = 400; ///< Number of VT bitfield transitions stored in the VideoRec::dwPoints array
const int VTRecNumTargets = 50; ///< Number of VT bitfield transitions stored in the VideoRec::dntargets array
const std::uint16_t VTRecSWST = 0x800; ///< Value always used for VideoRec::swstx

typedef struct 	{// from Neuralynx header Nlx_DataTypes.h
    std::uint16_t swstx; ///< Value is always NlxDataTypes::NLX_VTREC_SWSTX
    std::uint16_t swid;	///< The ID assigned to the video tracker that created this record
    std::uint16_t swdata_size; ///< The size of the VT record in bytes
    std::uint64_t qwTimeStamp; ///< Timestamp of this record in microseconds
    std::uint32_t dwPoints[VTRecNumTransitionBitfields]; ///< An array of bitfields encoding all threshold crossings in the video frame.
    std::int16_t sncrc; ///< Ignored, relic from Cheetah160VT
    std::int32_t dnextracted_x; ///< Calculated x coordinate from our extraction algorithm
    std::int32_t dnextracted_y;	///< Calculated y coordinate from our extraction algorithm
    std::int32_t dnextracted_angle; ///< Calculated head direction in degrees from the Y axis
    std::int32_t dntargets[VTRecNumTargets]; ///< An array of aggregated transitions in the same bitfield format as VideoRec::dwPoints
} __attribute__ ((__packed__)) VideoRec; // packing does not hurt perfomance on x64 CPUs 
// used here in order to guarantee sizeof(VideoRec) == swdata_size so that is read correctly (otherwise it is NOT!)
// read more on: https://attractivechaos.wordpress.com/2013/05/02/does-packed-struct-hurt-performance-on-x86_64/

struct ErrorNLXVT {//wrapped to avoid namespace conflict
    enum Code {
    UNKNOWN = -1,
    NO_ERROR = 0,
    SWSTX,
    SWID,
    SWDATA_SIZE,
    NEGATIVE_COORDINATE,
    OUT_OF_RESOLUTION
    };
};

const std::array<std::int32_t, 2> NLX_VIDEO_RESOLUTION = { {720, 576} };

bool valid_nlx_vt( VideoRec* vt_record, std::uint16_t vt_id,
    ErrorNLXVT::Code& error_code, decltype(NLX_VIDEO_RESOLUTION) resolution );

inline const unsigned int nlx_field_data_last( unsigned int nchannels = NLX_DEFAULT_NCHANNELS ) {
    
    return NLX_FIELD_DATA_FIRST + nchannels - 1;
}
inline const unsigned int nlx_field_crc( unsigned int nchannels = NLX_DEFAULT_NCHANNELS ) {
    
    return NLX_FIELD_DATA_FIRST + nchannels;
}
inline const int32_t nlx_packetsize( unsigned int nchannels = NLX_DEFAULT_NCHANNELS ) {
    
    return nchannels + 10;
}

class NlxSignalRecord {
public:
    NlxSignalRecord( unsigned int nchannels = NLX_DEFAULT_NCHANNELS );
    
    unsigned int nchannels();
    void set_nchannels( unsigned int n );
    
    bool FromNetworkBuffer( char * buffer, size_t n );
    size_t ToNetworkBuffer( char * buffer, size_t n );
    
    void Initialize(); // set required fields 1-3
    void Finalize(); // compute CRC
    
    int32_t crc();
    
    bool initialized();
    bool finalized();
    
    bool valid();
    
    // timestamp access functions
    uint64_t timestamp();
    void set_timestamp( uint64_t t );
    void inc_timestamp( uint64_t delta );
    void inc_timestamp( double delta );
    
    // digital input/output access methods
    uint32_t parallel_port();
    void set_parallel_port( uint32_t dio = 0 );
    
    // data (int32) getter methods
    void data( std::vector<int32_t>& v ); // will copy
    std::vector<int32_t>::iterator data( std::vector<int32_t>::iterator it ); // will copy
    int32_t sample( unsigned int index );
    
    // data (int32) setter methods
    void set_data( int32_t value = 0 );
    void set_data( std::vector<int32_t>& v );
    void set_data( std::vector<int32_t>::iterator it );
    
    // data (microVolt) getter methods
    void data( std::vector<double>& v ); // will convert to uV and copy
    std::vector<double>::iterator data( std::vector<double>::iterator it);
    double sample_microvolt( unsigned int index );
    
    // data (microVolt) setter methods
    void set_data( double value = 0 );
    void set_data( std::vector<double>& v );
    void set_data( std::vector<double>::iterator it );

protected:
    std::vector<int32_t>::iterator data_begin();
    std::vector<int32_t>::iterator data_end();
    
    unsigned int nchannels_;
    std::vector<int32_t> buffer_;
    bool initialized_ = false;
    bool finalized_ = false;
    
    unsigned int nlx_nfields_;
    unsigned int nlx_packetbytesize_;
    uint16_t nlx_field_crc_;
    uint16_t nlx_field_data_last_;
    int32_t nlx_packetsize_;
};


#endif // nlx.hpp
