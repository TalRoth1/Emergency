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
        std::cout << correctFrame << std::endl;
        
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
        std::cout << "subscribe command to " << channel<< " with subid "<< subId <<std::endl;  

        return true;
    }
    else if(input.getCommand() == "UNSUBSCRIBE")
    {
        if(!isLogin.load())
        {
            std::cout << "you need to login first" << std::endl;
            return false;
        }
        std::map<std::string, std::string> arg = input.getHeaders();
        std::string channel = arg["destination"];
        if (subscriptions.find(channel) == subscriptions.end()) {
            return true;// we dont stop the client
        }

        std::string subId = subscriptions[channel];
        input.getHeaders()["id"] = subId;
        subscriptions.erase(channel);
        connectionHandler -> sendFrameAscii(input.toString(), '\0');
        std::cout << "unsubscribe command to " << channel<< " with subid "<< subId <<std::endl; 
        return true;
    }
    else if(input.getCommand() == "DISCONNECT")
    {
        if(!isLogin.load())
        {
            std::cout << "you need to login first" << std::endl;
            return false;
        }
        isLogin.store(false);
        connectionHandler->close();
        std::cout << "protocol: disconnect command" << std::endl; 
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
        std::cout << "send command send to cH" << std::endl;   
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
    std::cout << "protocol: logout" << std::endl;
}
void StompProtocol::handleSummary(const Frame &frame) {
    // Read channel, user, file from headers
    const auto &h = frame.getHeaders();
    std::string channel = h.at("channel");
    std::string user    = h.at("user");
    std::string file    = h.at("file");

    std::cout << "StompProtocol::handleSummary => channel=" << channel
              << ", user=" << user << ", file=" << file << std::endl;

    auto key = std::make_pair(channel, user);
    if (receivedEvents.find(key) == receivedEvents.end()) {
        std::cout << "No events for channel=" << channel << ", user=" << user << std::endl;
        return;
    }

    auto &vec = receivedEvents[key];
    //need to ass sorting!!!!!!!!!!!!!!!!!!!!!
    std::ofstream outFile(file);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open file " << file << std::endl;
        return;
    }

    outFile << "Channel: " << channel << "\n"
            << "User: " << user << "\n"
            << "Total events: " << vec.size() << "\n\n";

    int idx = 1;
    for (auto &ev : vec) {
        outFile << "Event #" << idx++ << ":\n";
        outFile << "  city: " << ev.get_city() << "\n";
        outFile << "  event name: " << ev.get_name() << "\n";
        outFile << "  date time: " << epochToDate(ev.get_date_time()) << "\n";
        outFile << "  general information:\n";
        for (auto &info : ev.get_general_information()) {
            outFile << "    " << info.first << ": " << info.second << "\n";
        }
        outFile << "  description:\n" << ev.get_description() << "\n";
    }
    outFile.close();
    std::cout << "Summary has been written to " << file << std::endl;
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
        }   
    return true;      
}