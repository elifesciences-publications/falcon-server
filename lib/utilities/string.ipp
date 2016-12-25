template <typename T>
std::string to_string_n(const T a_value, const int n)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(n) << a_value;
    return out.str();
}
