#include "../include/StompClient.h"
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <sstream>
#include <chrono>

// =========== Constructors =========== //

StompClient::StompClient()
    : socket(ioContext),
      running(false)
{}

StompClient::StompClient(const std::string &host, short port)
    : socket(ioContext),
      running(false)
{
}

// =========== Destructor =========== //

StompClient::~StompClient()
{
    stop();
}

// =========== Connection logic =========== //
//
// New method that performs the actual socket connect, but does NOT
// start the threads or send CONNECT frame. Called from main AFTER login.

void StompClient::connectToServer(const std::string &host, short port) {
    // Use Boost ASIO to connect the socket
    boost::asio::ip::tcp::resolver resolver(ioContext);
    auto endpoints = resolver.resolve(host, std::to_string(port));
    boost::asio::connect(socket, endpoints);
}


// =========== Start / Stop =========== //
//
// Accepts username + password from main,
// then sends the CONNECT frame and launches threads.
void StompClient::start(const std::string &username, const std::string &password) {
    // Mark running
    running = true;

    // Immediately send a CONNECT frame with user credentials
    std::stringstream ss;
    ss << "CONNECT\n"
       << "accept-version:1.2\n"
       << "login:" << username << "\n"
       << "passcode:" << password << "\n"
       << "\n\0";
    sendToServer(ss.str());

    // Launch the threads
    commThread = std::thread(&StompClient::communicationThread, this);
    inputThread = std::thread(&StompClient::keyboardInputThread, this);
}

void StompClient::stop() {
    running = false;

    // Join threads if they were started
    if (commThread.joinable()) {
        commThread.join();
    }
    if (inputThread.joinable()) {
        inputThread.join();
    }
    socket.close();
}

// =========== Threads =========== //

void StompClient::communicationThread() {
    try {
        while (running) {
            // 1) Read messages from server
            std::string message = readMessageFromServer();
            if (!message.empty()) {
                processServerMessage(message);
            }

            // 2) Send any queued messages
            std::unique_lock<std::mutex> lock(queueMutex);
            // Wait until queue is not empty or we are no longer running
            queueCond.wait(lock, [this]() { return !messageQueue.empty() || !running; });

            while (!messageQueue.empty()) {
                std::string frame = messageQueue.front();
                messageQueue.pop();
                lock.unlock();

                sendToServer(frame);

                lock.lock();
            }
        }
    } catch (const std::exception &ex) {
        std::cerr << "Communication thread error: " << ex.what() << std::endl;
    }
}

void StompClient::keyboardInputThread() {
    while (running) {
        std::string input;
        if(!std::getline(std::cin, input)) {
            // If user closed stdin, break
            break;
        }
        sendCommand(input);
    }
}

// =========== Command Handling =========== //
//
// We removed the old "login " logic from here
// because login is now handled in main() before start().

void StompClient::sendCommand(const std::string &command) {
    std::string frame;

    if (boost::algorithm::starts_with(command, "join ")) {
        std::string channel = command.substr(5);
        frame = "SUBSCRIBE\ndestination:/" + channel + "\nid:1\n\n\0";

    } else if (boost::algorithm::starts_with(command, "exit ")) {
        frame = "UNSUBSCRIBE\nid:1\n\n\0";

    } else if (boost::algorithm::starts_with(command, "send ")) {
        std::string message = command.substr(5);
        frame = "SEND\ndestination:/topic\n\n" + message + "\0";

    } else if (command == "logout") {
        // Logging out => send DISCONNECT, then stop
        frame = "DISCONNECT\nreceipt:123\n\n\0";
        // Once we push this frame, the communication thread eventually sends it.
        // We will set running = false here so the client can stop gracefully
        running = false;

    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        return;
    }

    // Push the frame into the queue to be sent
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        messageQueue.push(frame);
        queueCond.notify_one();
    }
}

// =========== Low-level send/receive =========== //

void StompClient::sendToServer(const std::string &frame) {
    boost::asio::write(socket, boost::asio::buffer(frame));
}

std::string StompClient::readMessageFromServer() {
    boost::asio::streambuf buffer;
    boost::asio::read_until(socket, buffer, '\0'); // Read until null character
    std::istream is(&buffer);
    std::string message;
    std::getline(is, message, '\0'); // Extract the message
    return message;
}

// =========== Handling Incoming Messages =========== //

void StompClient::processServerMessage(const std::string &frame) {
    std::istringstream iss(frame);
    std::string command;
    std::getline(iss, command);

    if (command == "MESSAGE") {
        std::string destination, user, body;
        std::string line;
        bool inBody = false;

        while (std::getline(iss, line)) {
            if (inBody) {
                body += line + "\n";
            } else if (line.empty()) {
                inBody = true;
            } else if (boost::starts_with(line, "destination:")) {
                destination = line.substr(12);
            } else if (boost::starts_with(line, "user:")) {
                user = line.substr(5);
            }
        }
        boost::algorithm::trim(body);
        storeMessage(destination, user, body);
    }
}

// =========== Message Storage / Summary =========== //

void StompClient::storeMessage(const std::string &channel, const std::string &user, const std::string &message) {
    std::lock_guard<std::mutex> lock(storageMutex);
    messages[channel][user].push_back(message);
}

void StompClient::handleSummaryCommand(const std::string &channel, const std::string &user, const std::string &filename) {
    std::lock_guard<std::mutex> lock(storageMutex);

    auto channelIt = messages.find(channel);
    if (channelIt == messages.end()) {
        std::cout << "No messages for channel " << channel << std::endl;
        return;
    }

    auto userIt = channelIt->second.find(user);
    if (userIt == channelIt->second.end()) {
        std::cout << "No messages for user " << user << " in channel " << channel << std::endl;
        return;
    }

    std::ofstream outFile(filename);
    for (const auto &msg : userIt->second) {
        outFile << msg << std::endl;
    }
    std::cout << "Summary written to " << filename << std::endl;
}

// =========== Status =========== //

bool StompClient::isRunning() const {
    return running;
}
