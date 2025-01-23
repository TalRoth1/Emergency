#pragma once

#include <thread>
#include <atomic>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>

class keyboardInput
{
private:
    void listen();

    std::thread listenerThread;
    std::atomic<bool> isRunning;  
    std::queue<std::string> inputQueue;
    std::mutex queueMutex;
    std::condition_variable inputAvailable;

public:
    keyboardInput();
    ~keyboardInput();

    void start();

    void stop();

    bool getNextInput(std::string& input);
};