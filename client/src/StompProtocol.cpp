#include "../include/StompProtocol.h"

std::string StompProtocol::encodeFrame(const std::string &command, const std::string &body) {
    return command + "\n\n" + body + "\0";
}

std::string StompProtocol::decodeFrame(const std::string &frame) {
    return frame; // For simplicity, return raw frame. Add parsing logic as needed.
}
