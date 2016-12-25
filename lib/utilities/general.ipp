template<typename T, typename ...Args>
std::unique_ptr<T> make_unique( Args&& ...args ) {
    
    return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) );
}

template <class T, class A>
T join(const A &begin, const A &end, const T &t) {
    
  T result;
  A it = begin;
  
  if (it != end) { result.append(*it++); }

  for( ; it!=end; ++it) { result.append(t).append(*it); }
  
  return result;
}

