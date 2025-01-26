#pragma once
#include <string>
#include <map>

class Frame 
{
    private:
        std::string command;
        std::map<std::string, std::string> headers;
        std::string body;
    public:
        Frame(std::string command, std::map<std::string, std::string> headers, std::string body);
        ~Frame();
        std::string getCommand();
        std::map<std::string, std::string> getHeaders() const;
        std::string getBody();
        void addHeader(std::string key, std::string value);
        std::string toString();
        static Frame fromString(std::string str);
};