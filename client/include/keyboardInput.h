#pragma once
#include <thread>
#include <atomic>
#include "StompClient.h"

class KeyboardInput {
private:
    StompClient &client;
    std::atomic<bool> running;

public:
    KeyboardInput(StompClient &client);
    void start();
    void stop();
};
