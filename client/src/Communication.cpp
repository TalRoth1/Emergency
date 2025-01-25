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
    getThread = std::thread(&Communication::process, this);
    sendThread = std::thread(&Communication::receive, this);
}

void Communication::stop()
{
    if (isRunning.load())
    {
        isRunning.store(false);
        if (sendThread.joinable())
        {
            sendThread.join();
        }
        if (getThread.joinable())
        {
            getThread.join();
        }
    }
}

void Communication::process()
{
    while (isRunning.load())
    {
        Frame input = inputQueue->pop();
        bool worked = stompProtocol->process(input);
        if(input.getCommand() == "CONNECT" && worked)
        {
            isRunning.store(true);
        }
    }
}

void Communication::receive()
{
    while(isRunning.load())
    {
    this -> stompProtocol->receive();
    }
}
