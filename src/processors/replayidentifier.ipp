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

template <typename T1, typename T2>
void update_history( T1& container, T2 new_value) {
    
    std::size_t s = container.size();
    if (s <= 1) {
        throw std::range_error("Your container must have at least 2 elements!");
    }
    for (auto i=s-1; i>0; i--) {
        container[i] = container[i-1];
    }
    container[0] = static_cast<T2> (new_value);
}
