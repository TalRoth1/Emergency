#include "../include/Utilities.h"

#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <iostream>


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

std::string Utilities::translate(std::string &input, int subId, int receiptId)
{
    std::string frame = "";
    std::vector<std::string> arg = Utilities::splitString(input, ' ');
    std::string command = arg[0];
    if(command == "login")
    {
        if (arg.size() != 4)
        {
            std::cout << "login requiers: {host} {username} {password}" << std::endl;
        }
        else
        {
            frame += "CONNECT\n";
            frame += "accept-version:1.2\n";
            frame += "host:" + arg[1] + "\n";
            frame += "login:" + arg[2] + "\n";
            frame += "passcode:" + arg[3] + "\n";
            frame += "\n";
            frame += "\0";
        }
    }
    else if(command == "report")
    {
        if(arg.size() != 2)
        {
            std::cout << "report requiers: {file path}" << std::endl;
        }
        else
        {
            std::string data = Utilities::readJsonFileAsString(arg[1]);
            frame += "SEND\n";
        }

    }
    else if(command == "join")
    {
        if(arg.size() != 2)
        {
            std::cout << "join requiers: {channel name}" << std::endl;
        }
        else
        {
            frame += "SUBSCRIBE\n";
            frame += "destination:" + arg[1] + "\n";
            frame += "id:" + std::to_string(subId) + "\n";
            frame += "receipt:" + std::to_string(receiptId) + "\n";
            frame += "\n";
            frame += "\0";
        }
    }
    else if(command == "exit")
    {
        if(arg.size() != 2)
        {
            std::cout << "exit requiers: {channel name}" << std::endl;
        }
        frame += "UNSUBSCRIBE\n";
        frame += "destination:" + arg[1] + "\n";
        frame += "id:" + std::to_string(subId) + "\n";
        frame += "receipt:" + std::to_string(receiptId) + "\n";
        frame += "\n";
        frame += "\0";
    }
    else if(command == "logout")
    {
        if(arg.size() != 1)
        {
            std::cout << "too many arguments to logout" << std::endl;
        }
        else
        {
            frame += "DISCONNECT\n";
            frame += "receipt:" + std::to_string(receiptId) + "\n";
            frame += "\n";
            frame += "\0";
        }
    }
    else
    {
        std::cout << "Unkown Command" << std::endl;
    }

    return frame;
}

std::string Utilities::readJsonFileAsString(const std::string &filePath)
{
    std::ifstream inputFile(filePath);
    if (!inputFile.is_open())
    {
        throw std::runtime_error("Unable to open file: " + filePath);
    }
    std::ostringstream oss;
    oss << inputFile.rdbuf();
    return oss.str();
}

std::vector<Event> Utilities::parseIntoEvents(const std::string& jsonString)
{
    std::vector<Event> events;
    size_t  eventsStart = jsonString.find("\"events\": [");
}