<<<<<<< HEAD
#pragma once
#include "ConnectionHandler.h"
#include "StompProtocol.h"

class StompClient {
private:
    ConnectionHandler connectionHandler;
    StompProtocol stompProtocol;
    std::atomic<bool> running;

public:
    StompClient(const std::string &host, short port);
    void start();
    void stop();
    void sendCommand(const std::string &command);
};
=======
#ifndef STOMPCLIENT_H
#define STOMPCLIENT_H

#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <thread>
#include <mutex>
#include <queue>
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <condition_variable>

class StompClient {
public:
    StompClient(const std::string &host, short port);
    ~StompClient();

    void start();              // Start communication
    void stop();               // Stop communication
    void sendCommand(const std::string &command); // Send STOMP command
    void handleSummaryCommand(const std::string &channel, const std::string &user, const std::string &filename);

private:
    void communicationThread();         // Handle communication with server
    void keyboardInputThread();         // Handle user input
    void sendToServer(const std::string &frame); // Send frame to server
    std::string readMessageFromServer();         // Read frame from server
    void processServerMessage(const std::string &message); // Process server frames
    void storeMessage(const std::string &channel, const std::string &user, const std::string &message); // Store received message

    boost::asio::io_context ioContext;
    boost::asio::ip::tcp::socket socket;

    std::thread commThread;
    std::thread inputThread;

    std::mutex queueMutex;              // For message queue
    std::mutex storageMutex;            // For storing messages
    std::condition_variable queueCond; // Condition variable for queue

    std::queue<std::string> messageQueue; // Queue for outgoing messages
    std::map<std::string, std::map<std::string, std::vector<std::string>>> messages; // Messages storage

    bool running; // Client state
};

#endif
>>>>>>> Frame
