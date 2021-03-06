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

template <typename T>
T time2samples( double t, double rate, TruncateFlag flag ) {
    
    switch (flag) {
        case (TruncateFlag::FLOOR):
            return static_cast<T>( std::floor( t * rate ) );
        case (TruncateFlag::CEIL):
            return static_cast<T>( std::ceil( t * rate ) );
        default:
            return static_cast<T>( std::round( t * rate ) );
    }
}

template <typename T>
constexpr double samples2time( T nsamples, double rate ) {
    
    return static_cast<double>(nsamples/rate);
}
