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
    keyboardInput keyboard;
    ThreadSafeQueue queue;
    StompProtocol stompProtocol;
    Communication communication(&stompProtocol, &queue);
    keyboard.start();
    communication.start();
    bool flag = true;
    while(flag)
    {
        std::string input;
        if(keyboard.getNextInput(input))
        {
            if(input == "exit")
            {
                flag = false;
            }
        }
        Frame frame = Frame::fromString(input);
        queue.push(frame);
    }
    keyboard.stop();
    return 0;

}