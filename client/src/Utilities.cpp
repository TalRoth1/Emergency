#include "../include/Utilities.h"

#include <sstream>
#include <vector>
#include <string>

std::vector<std::string> Utilities::splitString(const std::string& str, char delimiter)
{
    std::vector<std::string> tokens;
    size_t start = 0, end;
    while ((end = str.find(delimiter, start)) != std::string::npos)
    {
        tokens.push_back(str.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(str.substr(start));
    return tokens;
}
void Utilities::split_str(const std::string &str, char delim, std::vector<std::string> &out)
{
    std::stringstream ss(str);
    std::string s;
    while (getline(ss, s, delim))
    {
        out.push_back(s);
    }
}