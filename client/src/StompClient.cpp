#include "../include/keyboardInput.h"
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
            if(input == "logout")/// suppose to be logout? 
            {
                flag = false;
                stompProtocol.logout(); 
                std::cout << "main : logout" << std::endl; 
            }
            std::string stringFrame = Utilities::translate(input, subId++, receiptId++);
            Frame frame = Frame::fromString(stringFrame);
            sendQueue.push(frame);
            std::cout << "push frame to queue" << frame.toString() << std::endl;  
        }
    }
    keyboard.stop();
    communication.stop();
    return 0;
}