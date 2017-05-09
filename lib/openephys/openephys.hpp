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

#ifndef OPENEPHYS_HPP
#define	OPENEPHYS_HPP

namespace OpenEphys {

const double SIGNAL_SAMPLING_FREQUENCY = 30000;
const double DSP_CUTOFF = 0.1;
const double LOWER_BANDWIDTH = 1;
const double UPPER_BANDWIDTH = 7500;
const bool DSP_ENABLED = true;
const double AD_BIT_MICROVOLTS = 0.195;
const int NCHANNELS_PER_PORT = 32;
const int DEFAULT_DATASTREAM = 0;
const double FOOT_CABLELENGTH = 6;

inline double ADbits_to_microvolts( int adbits ) {
    
    // how to convert to microvolts
    // https://open-ephys.atlassian.net/wiki/display/OEW/Flat+binary+format
    return AD_BIT_MICROVOLTS * adbits;
}

}

#endif	/* openephys.hpp */

