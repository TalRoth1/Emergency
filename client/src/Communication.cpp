#include "../include/Communication.h"

Communication::Communication(StompProtocol *stompProtocol, ThreadSafeQueue *inputQueue)
    : stompProtocol(stompProtocol), inputQueue(inputQueue), isRunning(false)
{
}

Communication::~Communication()
{
    stop();
}

void Communication::start()
{
    if (isRunning.load())
    {
        return;
    }
    isRunning.store(true);
    commThread = std::thread(&Communication::process, this);
}

void Communication::stop()
{
    if (isRunning.load())
    {
        isRunning.store(false);
        if (commThread.joinable())
        {
            commThread.join();
        }
    }
}

void Communication::process()
{
    while (isRunning.load())
    {
        Frame input = inputQueue->pop();
        stompProtocol->process(input);
    }
}
