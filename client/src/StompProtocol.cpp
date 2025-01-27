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
        if(isLogin.load())
        {
            std::cout << "The client is already logged in, log out before trying again." << std::endl;
            return true;
        }
        else
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
    }
    else if(input.getCommand() == "SUBSCRIBE")
    {
        if(!isLogin.load())
        {
            std::cout << "you need to login first" << std::endl;
            return true;
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
            return true;
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
            return true;
        }
        connectionHandler->sendFrameAscii(input.toString(), '\0');
        std::this_thread::sleep_for(std::chrono::seconds(2));
        logout();
    }
    else if(input.getCommand() == "SEND")
    {
        if(!isLogin.load())
        {
            std::cout << "you need to login first" << std::endl;
            return true;
        }
        connectionHandler->sendFrameAscii(input.toString(), '\0'); 
        return true;
    }
    else if(input.getCommand() == "SUMMARY")
    {
        if(!isLogin.load())
        {
            std::cout << "you need to login first" << std::endl;
            return true;
        }

        handleSummary(input);
    }
    else
    {
        return true;
    }
}

void StompProtocol::logout() {
    if (connectionHandler != nullptr) {
        connectionHandler->close();  // Close the connection
        delete connectionHandler;    // Free memory
        connectionHandler = nullptr; // Nullify the pointer
    }

    subscriptions.clear();          // Clear any subscriptions
    isLogin.store(false);           // Set login state to false
}


void StompProtocol::handleSummary(const Frame &frame) {
    // Read channel, user, file from headers
    const auto &h = frame.getHeaders();
    auto channelIt = h.find("channel");
    auto userIt = h.find("user");
    auto fileIt = h.find("file");

    // Check if the required headers exist
    if (channelIt == h.end() || userIt == h.end() || fileIt == h.end()) {
        std::cerr << "Missing required header(s)!" << std::endl;
        return;
    }

    std::string channel = channelIt->second;
    std::string user = userIt->second;
    std::string file = fileIt->second;

    auto key = std::make_pair(channel, user);
    auto it = receivedEvents.find(key);
    if (it == receivedEvents.end()) {
        std::cout << "No events for channel=" << channel << ", user=" << user << std::endl;
        return;
    }

    auto &vec = it->second;  // This is safe, since we found the key
    std::sort(vec.begin(), vec.end(), [](const Event &a, const Event &b)
    {
        // date_time ascending
        if (a.get_date_time() == b.get_date_time())
        {
            return a.get_name() < b.get_name();
        }
        return a.get_date_time() < b.get_date_time();
    });

    int total = static_cast<int>(vec.size());
    int activeCount = 0;
    int forcesCount = 0;

    for (auto &ev : vec)
    {
        // "active" is "true" or "false"
        auto activeIt = ev.get_general_information().find("active");
        if (activeIt != ev.get_general_information().end() && activeIt->second == "true") {
            activeCount++;
        }
        
        auto forcesIt = ev.get_general_information().find("forces_arrival_at_scene");
        if (forcesIt != ev.get_general_information().end() && forcesIt->second == "true") {
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
        return false;
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
    Frame frame = Frame::fromString(input);
    if(frame.getCommand() == "CONNECTED")
    {
        std::cout << "Login succsessful" << std::endl;
    }
    else if(frame.getCommand() == "ERROR")
    {
        std::cout << frame.getHeaders()["message"] << std::endl;
        logout();
    }
    else if(frame.getCommand() == "MESSAGE")
    {
        std::map<std::string, std::string> header = frame.getHeaders();
        std::string channel = header["destination"];
        channel = channel.substr(1);
        std::string body = frame.getBody();
        std::vector<std::string> lines = Utilities::splitString(body, '\n');
        std::string user = Utilities::splitString(lines[0], ':')[1];
        Event newEve = Event(body);
        std::pair<std::string, std::string> chanuser(channel, user);
        
        receivedEvents[chanuser].push_back(newEve);
    }
    else if(frame.getCommand() == "RECEIPT")
    {
        std::cout << frame.toString() << std::endl;
    }
    return true;      
}