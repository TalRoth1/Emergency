#pragma once

#include "../include/ConnectionHandler.h"
#include "../include/Frame.h"

// TODO: implement the STOMP protocol
class StompProtocol
{
private:
    bool isLogin;
    ConnectionHandler *connectionHandler;
    
public:
    StompProtocol();
    ~StompProtocol();

    void process(Frame &input);
};
