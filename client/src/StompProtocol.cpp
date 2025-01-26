#include "../include/StompProtocol.h"
#include "../include/Utilities.h"
#include "../include/event.h" // Add this line to include the definition of ReceivedEvent

#include <iostream>
#include <fstream>
// In StompProtocol.cpp
StompProtocol::StompProtocol()
 : isLogin(false)
 , connectionHandler(nullptr)
 , subscriptions()
 , receivedEvents() // etc.
{
}

StompProtocol::~StompProtocol()
{
    connectionHandler = nullptr;
}

bool StompProtocol::process(Frame &input)
{
    if(input.getCommand() == "CONNECT")
    {
        std::map<std::string, std::string> arg = input.getHeaders();
        std::string hostPort = arg["host"];
        std::string host = Utilities::splitString(hostPort, ':')[0];
        std::string port = Utilities::splitString(hostPort, ':')[1];
        this -> connectionHandler = new ConnectionHandler(host, std::stoi(port));
       
        std::string correctFrame = "CONNECT\naccept-version:1.2\nhost:stomp.cs.bgu.ac.il\nlogin:" + arg["login"] +"\npasscode:" + arg["passcode"] + "\n\n\0";
        
        if (!connectionHandler->connect()) {
            std::cerr << "Failed to connect to server." << std::endl;
            return false;
        }
        connectionHandler -> sendFrameAscii(correctFrame, '\0');
        isLogin.store(true);
        return true;
    }
    else if(input.getCommand() == "SUBSCRIBE")
    {
        if(!isLogin.load())
        {
            std::cout << "you need to login first" << std::endl;
            return false;
        }
        std::map<std::string, std::string> arg = input.getHeaders();
        std::string channel = arg["destination"];
        std::string subId = arg["id"];

        
        if (subscriptions.find(channel) != subscriptions.end()) {
            std::cout << "Error: Already subscribed to channel '" << channel << "'." << std::endl;
            return true;// not stop the client 
        }
       // subscriptions[channel] = subId;
        subscriptions.emplace(channel, subId);
        connectionHandler -> sendFrameAscii(input.toString(), '\0');

        return true;
    }
    else if(input.getCommand() == "UNSUBSCRIBE")// here was updated by me
    {
        if(!isLogin.load())
        {
            std::cout << "you need to login first" << std::endl;
            return false;
        }
        else{
        std::map<std::string, std::string> arg = input.getHeaders();
        std::string idToRemove = arg["id"];
       
        for (auto it = subscriptions.begin(); it != subscriptions.end(); ++it) {
            if (it->second == idToRemove) {
        // Found the channel that was subscribed with subId = idToRemove
                subscriptions.erase(it);
                break; // done removing from the map
            }
        }
        connectionHandler -> sendFrameAscii(input.toString(), '\0');
        return true;
        }
    
    } // untill here
    else if(input.getCommand() == "DISCONNECT")
    {
        if(!isLogin.load())
        {
            std::cout << "you need to login first" << std::endl;
            return false;
        }
        connectionHandler->sendFrameAscii(input.toString(), '\0');
        return true;
    }
    else if(input.getCommand() == "SEND")
    {
        if(!isLogin.load())
        {
            std::cout << "you need to login first" << std::endl;
            return false;
        }
        connectionHandler->sendFrameAscii(input.toString(), '\0'); 
        return true;
    }
    else if(input.getCommand() == "SUMMARY")
    {
        if(!isLogin.load())
        {
            std::cout << "you need to login first" << std::endl;
            return false;
        }

        handleSummary(input);
        std::cout<< "Summary command recieved" << std::endl;// where is actually handlded?
    }
    else
    {
        return false;
    }
}
    
void StompProtocol::logout()
{
    isLogin.store(false);
     if (connectionHandler) {
        connectionHandler->close();
     }
    subscriptions.clear();
}
void StompProtocol::handleSummary(const Frame &frame) {
    // Read channel, user, file from headers
    const auto &h = frame.getHeaders();
    std::string channel = h.at("channel");
    std::string user = h.at("user");
    std::string file = h.at("file");

    auto key = std::make_pair(channel, user);
    if (receivedEvents.find(key) == receivedEvents.end()) {
        std::cout << "No events for channel=" << channel << ", user=" << user << std::endl;
        return;
    }
 
    auto &vec = receivedEvents[key];
    std::sort(vec.begin(), vec.end(), [](const Event &a, const Event &b)
    {
    // date_time ascending
        if (a.get_date_time() == b.get_date_time())
        {
            return a.get_name() < b.get_name();
        }
        return a.get_date_time() < b.get_date_time();
    });
    int total = (int)vec.size();
    int activeCount = 0;
    int forcesCount = 0;

    for (auto &ev : vec)
    {
        // "active" is "true" or "false"
        if (ev.get_general_information().at("active") == "true") {
            activeCount++;
        }
        if (ev.get_general_information().at("forces_arrival_at_scene") == "true") {
            forcesCount++;
        }
    }
    std::ofstream outFile(file);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open file " << file << std::endl;
        return;
    }
    outFile << "Channel " << channel << "\n"
            << "Stats:\n"
            << "Total: " << total << "\n"
            << "active: " << activeCount << "\n"
            << "forces arrival at scene: " << forcesCount << "\n\n";
    outFile << "Event Reports:\n";
    int idx = 1;
    for (auto &ev : vec) {
        outFile << "Report_" << idx++ << ":\n";
        outFile << "  city: " << ev.get_city() << "\n";
        outFile << "  date time: " << epochToDate(ev.get_date_time()) << "\n";
        outFile << "  event name: " << ev.get_name() << "\n";
        std::string desc = ev.get_description();
        if (desc.size() > 30)
        {
            std::string partial = desc.substr(0, 27) + "...";
            outFile << "  summary: " << partial << "\n";
        } else {
            outFile << "  summary: " << desc << "\n";
        }
        outFile << "\n";
        }
    outFile.close();
    std::cout << "Summary written to " << file << std::endl;
}

std::string StompProtocol::epochToDate(int epoch) {
    time_t t = (time_t) epoch;
    struct tm *lt = localtime(&t);
    char buf[64];
    strftime(buf, sizeof(buf), "%d/%m/%y %H:%M", lt);
    return std::string(buf);
}
bool StompProtocol::receive()
{
    if(!isLogin.load())
    {
        return true;
    }
     if (connectionHandler == nullptr) {
        // If there's no connectionHandler, can't read.
        return false;
    }

    std::string input;
   
    if(!connectionHandler -> getFrameAscii(input, '\0'))
    {
        return false;
    }
    std::cout << input << std::endl;
    Frame frame = Frame::fromString(input);
    if(frame.getCommand() == "MESSAGE")
    {
        std::map<std::string, std::string> header = frame.getHeaders();
        std::string channel = header["destination"];
        std::string body = frame.getBody();
         std::cout << "Message recieved" << header["destination"] << std::endl;
        std::cout << body << std::endl;
    }   
    return true;      
}