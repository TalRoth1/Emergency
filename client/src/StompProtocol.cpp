#include "../include/StompProtocol.h"
#include "../include/Utilities.h"

#include <iostream>


StompProtocol::StompProtocol():isLogin(false)
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
        std::cout << "disconnect command" << std::endl; 
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
        std::cout<< "Summary command recieved" << std::endl;// where is actually handlded?
        return true;
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