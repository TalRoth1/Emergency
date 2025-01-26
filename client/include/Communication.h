#pragma once

#include <thread>
#include <atomic>
#include <string>
#include "StompProtocol.h"
#include "ThreadSafeQueue.h"

class Communication
{
private:
    StompProtocol *stompProtocol;
    std::atomic<bool> isRunning;  
    std::thread sendThread;
    std::thread getThread;
    ThreadSafeQueue *inputQueue;

    void process();
    void receive();

public:
    Communication(StompProtocol *stompProtocol, ThreadSafeQueue *inputQueue);
    ~Communication();

    void start();
    void stop();
};
