#ifndef KEYBOARDINPUT_H
#define KEYBOARDINPUT_H

#include "StompClient.h"

class KeyboardInput {
public:
    KeyboardInput(StompClient &client);
    void start(); // Start reading input from user
    void stop();  // Stop reading input from user

private:
    StompClient &client;
    bool running; // State of the input reader
};

#endif
