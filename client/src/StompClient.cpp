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
        connectionHandler -> connect();
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
        subscriptions.emplace(channel, subId);
        connectionHandler -> sendFrameAscii(input.toString(), '\0');
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
        std::string subId = subscriptions[channel];
        input.getHeaders()["id"] = subId;
        subscriptions.erase(channel);
        connectionHandler -> sendFrameAscii(input.toString(), '\0');
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
        std::cout<< "Summary recieved" << std::endl;
        return true;
    }
    else
    {
        return false;
    }
}

void StompProtocol::receive()
{
    if(!isLogin.load())
    {
        return;
    }
    std::string input;
    if (connectionHandler != nullptr)
    {
        if(connectionHandler -> getFrameAscii(input, '\0'))
        {
            std::cout << input << std::endl;
            Frame frame = Frame::fromString(input);
            if(frame.getCommand() == "MESSAGE")
            {
                std::map<std::string, std::string> header = frame.getHeaders();
                std::string channel = header["destination"];
            }
        }
    }
}