/*
 Utilities to manipulate, create and check string data and other containers with strings.
 */

#ifndef STRING_HPP
#define	STRING_HPP

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>

bool path_exists (const std::string& name);

std::vector<std::string> &split(
    const std::string &s,
    char delim,
    std::vector<std::string> &elems);

std::vector<std::string> split(const std::string &s, char delim);

std::string resolve_server_path(
    std::string p,
    const std::map<std::string,
    std::string>& contexts,
    std::string default_context = "" );

template <typename T>
std::string to_string_n(const T a_value, const int n = 1);

// removes the appended (complete or incomplete) filename from the path
// and returns the path to the folder;
// returned path ends with '/'
std::string extract_path_to_folder( std::string path_to_file );

// 
void complete_path( std::string& file_path, std::string processor_name, std::string extension );

template<typename T>
inline std::string get_type_string() { return "unknown"; }
template<>
inline std::string get_type_string<bool>() { return "bool"; }
template<>
inline std::string get_type_string<double>() { return "float64"; }
template<>
inline std::string get_type_string<float>() { return "float32"; }
template<>
inline std::string get_type_string<int8_t>() { return "int8"; }
template<>
inline std::string get_type_string<uint8_t>() { return "uint8"; }
template<>
inline std::string get_type_string<int16_t>() { return "int16"; }
template<>
inline std::string get_type_string<uint16_t>() { return "uint16"; }
template<>
inline std::string get_type_string<int32_t>() { return "int32"; }
template<>
inline std::string get_type_string<uint32_t>() { return "uint32"; }
template<>
inline std::string get_type_string<int64_t>() { return "int64"; }
template<>
inline std::string get_type_string<uint64_t>() { return "uint64"; }

#include "string.ipp"

#endif	// string.hpp

