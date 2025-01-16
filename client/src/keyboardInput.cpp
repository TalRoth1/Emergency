#include "../include/keyboardInput.h"
#include <iostream>

KeyboardInput::KeyboardInput(StompClient &client) : client(client), running(true) {}

void KeyboardInput::start() {
    std::thread([this]() {
        while (running) {
            std::string command;
            std::getline(std::cin, command);
            client.sendCommand(command);
        }
    }).detach();
}

void KeyboardInput::stop() {
    running = false;
}
