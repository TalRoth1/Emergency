#include "../include/keyBoardInput.h"
#include "../include/Utilities.h"
#include "../include/ThreadSafeQueue.h"
#include "../include/ConnectionHandler.h"
#include "../include/Communication.h"
#include "../include/Frame.h"

#include <string>
#include <iostream>

int main(int argc, char *argv[])
{
    int subId = 0;
    int receiptId = 0;
    keyboardInput keyboard;
    ThreadSafeQueue sendQueue;
    StompProtocol stompProtocol;
    Communication communication(&stompProtocol, &sendQueue);
    keyboard.start();
    communication.start();
    bool flag = true;
    while(flag)
    {
        std::string input;
        if(keyboard.getNextInput(input))
        {
            std::string stringFrame = Utilities::translate(input, subId++, receiptId++);
            std::vector<std::string> arg = Utilities::splitString(input, ' ');
            if(arg[0] == "report")
            {
                std::vector<std::string> eventFrames = Utilities::splitString(stringFrame, '\0');
                for (std::string event : eventFrames)
                {
                    Frame frame = Frame::fromString(event);
                    sendQueue.push(frame);
                }
            }
            else if(!stringFrame.empty())
            {
                Frame frame = Frame::fromString(stringFrame);
                sendQueue.push(frame);
            }
        }
    }
    keyboard.stop();
    communication.stop();
    return 0;
}