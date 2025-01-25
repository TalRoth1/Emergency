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

    // Find the channel name
    size_t channelNameStart = jsonString.find("\"channel_name\":");
    size_t channelNameValueStart = jsonString.find("\"", channelNameStart + 15) + 1;
    size_t channelNameValueEnd = jsonString.find("\"", channelNameValueStart);
    std::string channelName = jsonString.substr(channelNameValueStart, channelNameValueEnd - channelNameValueStart);

    // Find the start of the "events" array
    size_t eventsStart = jsonString.find("\"events\": [", channelNameValueEnd);
    size_t eventsEnd = jsonString.find("]", eventsStart);

    size_t eventStart = jsonString.find("{", eventsStart);
    while (eventStart != std::string::npos && eventStart < eventsEnd) {
        // Find the end of this event
        size_t eventEnd = jsonString.find("}", eventStart);

        // Parse individual fields of the event
        size_t eventNameStart = jsonString.find("\"event_name\":", eventStart);
        size_t eventNameValueStart = jsonString.find("\"", eventNameStart + 13) + 1;
        size_t eventNameValueEnd = jsonString.find("\"", eventNameValueStart);
        std::string eventName = jsonString.substr(eventNameValueStart, eventNameValueEnd - eventNameValueStart);

        size_t cityStart = jsonString.find("\"city\":", eventNameValueEnd);
        size_t cityValueStart = jsonString.find("\"", cityStart + 8) + 1;
        size_t cityValueEnd = jsonString.find("\"", cityValueStart);
        std::string city = jsonString.substr(cityValueStart, cityValueEnd - cityValueStart);

        size_t dateTimeStart = jsonString.find("\"date_time\":", cityValueEnd);
        size_t dateTimeValueStart = jsonString.find(":", dateTimeStart) + 1;
        size_t dateTimeValueEnd = jsonString.find(",", dateTimeValueStart);
        int dateTime = std::stoi(jsonString.substr(dateTimeValueStart, dateTimeValueEnd - dateTimeValueStart));

        size_t descriptionStart = jsonString.find("\"description\":", dateTimeValueEnd);
        size_t descriptionValueStart = jsonString.find("\"", descriptionStart + 14) + 1;
        size_t descriptionValueEnd = descriptionValueStart;

        // Handle escaped characters and find the true end of the description
        std::string description;
        while (descriptionValueEnd < jsonString.size()) {
            descriptionValueEnd = jsonString.find("\"", descriptionValueEnd);
            if (descriptionValueEnd == std::string::npos || jsonString[descriptionValueEnd - 1] != '\\') {
                break; // Found the correct end of the description
            }
            descriptionValueEnd++; // Skip the escaped quote
        }
        description = jsonString.substr(descriptionValueStart, descriptionValueEnd - descriptionValueStart);

        // Replace escaped characters (e.g., \" and \n)
        for (size_t pos = description.find("\\"); pos != std::string::npos; pos = description.find("\\", pos)) {
            if (description[pos + 1] == 'n') {
                description.replace(pos, 2, "\n");
            } else if (description[pos + 1] == '\"') {
                description.erase(pos, 1); // Remove the escape for quotes
            } else {
                description.erase(pos, 1); // Remove the backslash for other characters
            }
        }

        size_t generalInfoStart = jsonString.find("\"general_information\":", descriptionValueEnd);
        size_t generalInfoEnd = jsonString.find("}", generalInfoStart);
        std::map<std::string, std::string> generalInformation;

        size_t activeStart = jsonString.find("\"active\":", generalInfoStart);
        size_t activeValueStart = jsonString.find(":", activeStart) + 1;
        size_t activeValueEnd = jsonString.find(",", activeValueStart);
        std::string active = jsonString.substr(activeValueStart, activeValueEnd - activeValueStart);
        generalInformation["active"] = active.find("true") != std::string::npos ? "true" : "false";

        size_t forcesArrivalStart = jsonString.find("\"forces_arrival_at_scene\":", activeValueEnd);
        size_t forcesArrivalValueStart = jsonString.find(":", forcesArrivalStart) + 1;
        size_t forcesArrivalValueEnd = jsonString.find("}", forcesArrivalValueStart);
        std::string forcesArrival = jsonString.substr(forcesArrivalValueStart, forcesArrivalValueEnd - forcesArrivalValueStart);
        generalInformation["forces_arrival_at_scene"] = forcesArrival.find("true") != std::string::npos ? "true" : "false";

        // Create the Event object and add it to the vector
        Event event(channelName, city, eventName, dateTime, description, generalInformation);
        events.push_back(event);

        // Find the next event
        eventStart = jsonString.find("{", eventEnd);
    }

    return events;
}
