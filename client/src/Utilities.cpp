#include "../include/Utilities.h"

#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <iostream>

std::map<std::string,int> Utilities::channelToSubId;
std::map<int,std::string> Utilities::subIdToChannel;

int Utilities::nextSubId = 1;
int Utilities::nextReceiptId = 1;
std::string Utilities::currentUser = "";

std::vector<std::string> Utilities::splitString(const std::string& str, char delimiter)
{
    std::vector<std::string> tokens;
    size_t start = 0,  end = 0;
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
    std::string result = "";
    std::vector<std::string> arg = Utilities::splitString(input, ' ');
    if (arg.empty()) {
        std::cerr << "Empty command." << std::endl;
        return ""; // empty
    }
    
    std::string command = arg[0];
    
    if(command == "login")
    {
        if (arg.size() != 4)
        {
            std::cout << "login requiers: {host} {username} {password}" << std::endl;
            return ""; // or do simthing else? 
        }
        else
        {
            Utilities::currentUser = arg[2];  
            int receipt = nextReceiptId++;
            result += "CONNECT\n";
            result += "accept-version:1.2\n";
            result += "host:" + arg[1] + "\n";
            result += "login:" + arg[2] + "\n";
            result += "passcode:" + arg[3] + "\n";
            result += "\n";
            result += "\0";
            return result;
        }
    }
    else if(command == "report")
    {
        if(arg.size() != 2)
        {
            std::cout << "report requiers: {file path}" << std::endl;
            return "";
        }
        else
        {
        std::string jsonPath = arg[1];
        std::string jsonString;
        try 
        {
            jsonString = Utilities::readJsonFileAsString(jsonPath);
        } 
        catch(const std::exception &e) 
        {
            std::cerr << "Failed to read file: " << e.what() << std::endl;
            return "";
        }

        std::vector<Event> events = Utilities::parseIntoEvents(jsonString);
            for (auto &ev : events) {
            result+= "SEND\n";
            result+= "destination:/" + ev.get_channel_name() + "\n\n";
            result+= "user:" + currentUser + "\n";
            result+= "city:" + ev.get_city() + "\n";
            result+= "event name:" + ev.get_name() + "\n";
            result+= "date time:" + std::to_string(ev.get_date_time()) + "\n";
            result+= "general information:\n";
            result+= "  active: " + ev.get_general_information().at("active") + "\n";
            result+= "  forces_arrival_at_scene: " + ev.get_general_information().at("forces_arrival_at_scene") + "\n";
            result+= "description:\n";
            result+= ev.get_description() + '\n' + '\0';
            }
            return result;
        }       
    }
    else if(command == "join")
    {
        if(arg.size() != 2)
        {
            std::cout << "join requiers: {channel name}" << std::endl;
            return "";

        }
        std::string channel = arg[1];

        // If not already subscribed, create a new subId
        if (channelToSubId.find(channel) == channelToSubId.end()) {
            channelToSubId[channel] = nextSubId;
            subIdToChannel[nextSubId] = channel;
            nextSubId++;
        } 
            subId = channelToSubId[(arg[1])];
            result += "SUBSCRIBE\n";
            result += "destination:" + arg[1] + "\n";
            result += "id:" + std::to_string(subId) + "\n";
            result += "receipt:" + std::to_string(receiptId) + "\n";
            result += "\n";
            result += "\0";
            return result;
    }
    else if(command == "exit")
    {
        if(arg.size() != 2)
        {
            std::cout << "exit requiers: {channel name}" << std::endl;
            return "";
        }
        std::string channel = arg[1];

        if (channelToSubId.find(channel) == channelToSubId.end()) {
            std::cerr << "Error: Not subscribed to channel '" << channel << "'." << std::endl;
            return ""; // no frame
        }
        subId = channelToSubId[channel];
        channelToSubId.erase(channel);
        subIdToChannel.erase(subId);
        int receipt = nextReceiptId++;


        result += "UNSUBSCRIBE\n";
        // result += "destination:" + arg[1] + "\n"; not suppose to be 
        result += "id:" + std::to_string(subId) + "\n";
        result += "receipt:" + std::to_string(receiptId) + "\n";
        result += "\n";
        result += "\0";
        return result;  
    }
    else if(command == "logout")
    {
        if(arg.size() != 1)
        {
            std::cout << "too many arguments to logout" << std::endl;
            return "";
        }
        else
        {
            result += "DISCONNECT\n";
            result += "receipt:" + std::to_string(receiptId) + "\n";
            result += "\n";
            result += "\0";   
            return result;
        }
    }
    
    else if (command == "summary") {
        if (arg.size() != 4) {
            std::cerr << "summary requires {channel} {user} {file}\n";
            return "";
        }
        else
        {
            result+= "SUMMARY\n";    
            result+= "channel:" + arg[1] + "\n";
            result+= "user:" + arg[2] + "\n";
            result+= "file:" + arg[3] + "\n";
            result+= "\n";
            result+= "\0";
            return result;
        }
    }

    else
        {
            std::cout << "Unkown Command" << std::endl;
        }

    return "";
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
