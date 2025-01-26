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
         if (!worked) {
            isRunning.store(false);
        }
        // If the command was DISCONNECT, you can also force isRunning = false
        if (input.getCommand() == "DISCONNECT") {
            isRunning.store(false);
        }
    }
}

void Communication::receive()
{
    while (isRunning.load())
    {
        bool success = stompProtocol->receive();
        if (!success) {
            std::cerr << "Connection closed or read error. Stopping receive loop.\n";
            // You should close the connection here using connectionHandler->close()
            // if you haven't already closed it. Or do it inside receive().

            // Mark isRunning as false so that the process() thread also sees it.
            stompProtocol-> logout();

            isRunning.store(false);
            break;
        }
    }
}
