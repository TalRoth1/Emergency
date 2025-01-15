#pragma once

#include "../include/ConnectionHandler.h"

// TODO: implement the STOMP protocol
class StompProtocol
{
private:
public:
    std::string encodeFrame(const std::string &command, const std::string &body);
    std::string decodeFrame(const std::string &frame);
};
