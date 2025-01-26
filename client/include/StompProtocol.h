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
    std::map<std::pair<std::string, std::string>, std::vector<Event>> receivedEvents;
    
public:
    StompProtocol();
    ~StompProtocol();

    bool process(Frame &input);
    bool receive();//change to bool
    void logout();
    static std::string epochToDate(int epoch);
    void handleSummary(const Frame &frame); 
};
