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
