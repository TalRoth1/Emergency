#pragma once
#include "ConnectionHandler.h"
#include "StompProtocol.h"

class StompClient {
private:
    ConnectionHandler connectionHandler;
    StompProtocol stompProtocol;
    std::atomic<bool> running;

public:
    StompClient(const std::string &host, short port);
    void start();
    void stop();
    void sendCommand(const std::string &command);
};
