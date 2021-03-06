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

#ifndef VECTOR_IO_H
#define VECTOR_IO_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iterator>

template <typename T>
void read_vector_from_binary( std::ifstream & stream, std::vector<T> & vec, std::size_t n =0 );

template <typename T>
std::vector<T> read_vector_from_binary( std::string filename );

template <typename T>
void save_vector_to_binary( std::ofstream & stream, std::vector<T> & vec );

template <typename T>
void save_vector_to_binary( std::string filename, std::vector<T> & vec );

template <typename T>
void read_vector_from_text( std::ifstream & stream, std::vector<T> & vec, std::size_t n =0 );

template <typename T>
std::vector<T> read_vector_from_text( std::string filename );

template <typename T>
void save_vector_to_text( std::ofstream & stream, std::vector<T> & vec );

template <typename T>
void save_vector_to_text( std::string filename, std::vector<T> & vec );

#include "vector_io.ipp" // implementation of templated functions

#endif
