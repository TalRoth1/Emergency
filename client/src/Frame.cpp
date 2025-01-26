#include "../include/Frame.h"

#include <sstream>
#include <iostream>


Frame::Frame(std::string command, std::map<std::string, std::string> headers, std::string body)
{
    this->command = command;
    this->headers = headers;
    this->body = body;
}
Frame::~Frame()
{
}

std::string Frame::getCommand()
{
    return this->command;
}

std::map<std::string, std::string> Frame::getHeaders() const // changes to const
{
    return this->headers;
}

std::string Frame::getBody()
{
    return this->body;
}

void Frame::addHeader(std::string key, std::string value)
{
    this->headers[key] = value;
}

std::string Frame::toString()
{
    std::string str = this -> command + "\n";
    for (std::map<std::string, std::string>::iterator it = this->headers.begin(); it != this->headers.end(); it++)
    {
        str += it->first + ":" + it->second + "\n";
    }
    str += "\n" + this->body + "\0";
    return str;
}

Frame Frame::fromString(std::string str)
{
    std::string command;
    std::map<std::string, std::string> headers;
    std::string body;
    std::string line;
    std::istringstream stream(str);
    std::getline(stream, command);
    while (std::getline(stream, line) && line != "")
    {
        std::string key = line.substr(0, line.find(":"));
        std::string value = line.substr(line.find(":") + 1);
        headers[key] = value;
    }
    std::getline(stream, body, '\0');
    return Frame(command, headers, body);
}