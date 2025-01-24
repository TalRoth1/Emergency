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
    if(input.getCommand() == "CONNECTED")
    {
        std::map<std::string, std::string> arg = input.getHeaders();
        std::string hostPort = arg["host"];
        std::string host = Utilities::splitString(hostPort, ':')[0];
        std::string port = Utilities::splitString(hostPort, ':')[1];
        this -> connectionHandler = new ConnectionHandler(host, std::stoi(port));
        connectionHandler -> connect();
        connectionHandler -> sendFrameAscii(input.toString(), '\0');
        isLogin.store(true);
        return true;
    }
    else if(input.getCommand() == "SUMMARY")
    {
           std::cout<< "Summary recieved" << std::endl;
           return true;
    }
    else
    {
        if(!isLogin.load())
        {
            std::cout << "you need to login first" << std::endl;
            return false;
        }
        else
        {
            connectionHandler -> sendFrameAscii(input.toString(), '\0');
            return true;
        }
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