#pragma once

#include <vector>
#include <string>

class Utilities
{
    public:
        static std::vector<std::string> splitString(const std::string& str, char delimiter);
        static void split_str(const std::string &str, char delim, std::vector<std::string> &out);
};