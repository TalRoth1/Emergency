#pragma once

#include "../include/ConnectionHandler.h"
#include "../include/Frame.h"
#include "../include/event.h"

#include <map>

// TODO: implement the STOMP protocol
class StompProtocol
{
private:
    std::atomic<bool> isLogin; 
    ConnectionHandler *connectionHandler;
    std::map<std::string, std::string> subscriptions;
    std::map<std::string, std::map <std::string, std::vector<Event>>> reports; // <Channel, <Username, Report>>
    
public:
    StompProtocol();
    ~StompProtocol();

    bool process(Frame &input);
    bool receive();//change to bool
    void logout();
};
