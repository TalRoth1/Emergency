#include "../include/keyBoardInput.h"

#include <iostream>

keyboardInput::keyboardInput() : isRunning(false)
{}

keyboardInput::~keyboardInput()
{
    stop();
}

void keyboardInput::start()
{
    if (isRunning) {
        return;
    }
    isRunning = true;
    listenerThread = std::thread(&keyboardInput::listen, this);
}

void keyboardInput::stop()
{
    if (isRunning) {
        isRunning = false;
        if (listenerThread.joinable()) {
            listenerThread.join();
        }
    }
}

bool keyboardInput::getNextInput(std::string& input)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    if (!inputQueue.empty()) {
        input = inputQueue.front();
        inputQueue.pop();
        return true;
    }
    return false;
}

void keyboardInput::listen()
{
    while (isRunning) {
        std::string input;
        std::getline(std::cin, input);
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            inputQueue.push(input);
        }
        inputAvailable.notify_one();
    }
}