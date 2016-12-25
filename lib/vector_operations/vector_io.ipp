

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
