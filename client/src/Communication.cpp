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
    isRunning.store(true);
    getThread = std::thread(&Communication::process, this);
    sendThread = std::thread(&Communication::receive, this);
}

void Communication::stop()
{
    if (isRunning.load())
    {
        isRunning.store(false);
        if (getThread.joinable())
        {
            getThread.detach();
        }
        if (sendThread.joinable())
        {
            sendThread.detach();
        }
    }
}
void Communication::updateReceive()
{
    if(getThread.joinable())
        getThread.join();
    getThread = std::thread(&Communication::receive, this);
}

void Communication::process()
{
    while (isRunning.load())
    {
        Frame input = inputQueue->pop();
        bool worked = stompProtocol->process(input);
    }
}

void Communication::receive()
{
    while (isRunning.load())
    {
        bool success = stompProtocol->receive();
    }
}
