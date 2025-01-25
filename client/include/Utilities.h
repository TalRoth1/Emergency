#pragma once

#include "../include/event.h"
#include <vector>
#include <string>

class Utilities
{
    public:
        static std::vector<std::string> splitString(const std::string& str, char delimiter);
        static void split_str(const std::string &str, char delim, std::vector<std::string> &out);
        static std::string translate(std::string &input, int subId, int receiptId);
        static std::string readJsonFileAsString(const std::string &filePath);
        static std::vector<Event> parseIntoEvents(const std::string &jsonString);
        static std::map<std::string,int> channelToSubId;
        static std::map<int,std::string> subIdToChannel;
        static int nextSubId;
        static int nextReceiptId;   


};