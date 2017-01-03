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
void read_vector_from_binary( std::ifstream & stream, std::vector<T> & vec, std::size_t n) {
    
    if (n==0) {
        std::size_t start = stream.tellg();
        stream.seekg(0, std::ios_base::end);
        std::size_t end = stream.tellg();
        stream.seekg(start);
        n = (end-start)/sizeof(T);
    }
    
    vec.resize( n );
    
    stream.read( (char*) &vec[0], n*sizeof(T) );
    
}

template <typename T>
std::vector<T> read_vector_from_binary( std::string filename ) {
    std::ifstream stream(filename, std::ios::binary);
    std::vector<T> vec;
    read_vector_from_binary( stream, vec );
    stream.close();
    return vec;
}

template <typename T>
void save_vector_to_binary( std::ofstream & stream, std::vector<T> & vec ) {
    
    stream.write( (char*) &vec[0], vec.size()*sizeof(T) );
    
    //std::copy(vec.begin(), vec.end(), std::ostreambuf_iterator<double>(stream));
}

template <typename T>
void save_vector_to_binary( std::string filename, std::vector<T> & vec ) {
    std::ofstream stream(filename, std::ios::binary);
    save_vector_to_binary( stream, vec );
    stream.close();
}

template <typename T>
void read_vector_from_text( std::ifstream & stream, std::vector<T> & vec, std::size_t n) {
    
    T value;
    
    if (n==0) {
        while ( stream >> value) { vec.push_back( value ); }
    } else {
        while ( 0<n-- && stream>>value ) { vec.push_back(value); }
    }
}

template <typename T>
std::vector<T> read_vector_from_text( std::string filename ) {
    std::ifstream stream(filename);
    std::vector<T> vec;
    read_vector_from_text( stream, vec );
    stream.close();
    return vec;
}

template <typename T>
void save_vector_to_text( std::ofstream & stream, std::vector<T> & vec ) {
    std::ostream_iterator<T> stream_iterator(stream, "\n");
    std::copy(vec.begin(), vec.end(), stream_iterator);
}

template <typename T>
void save_vector_to_text( std::string filename, std::vector<T> & vec ) {
    std::ofstream stream( filename );
    save_vector_to_text( stream, vec );
    stream.close();
}
