#include "../include/StompClient.h"
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>


StompClient::StompClient(const std::string &host, short port)
    : socket(ioContext), running(false) {
    boost::asio::ip::tcp::resolver resolver(ioContext);
    auto endpoints = resolver.resolve(host, std::to_string(port));
    boost::asio::connect(socket, endpoints);
}

StompClient::~StompClient()
{
    stop();
}

void StompClient::start()
{
    running = true;

    // Launch threads
    commThread = std::thread(&StompClient::communicationThread, this);
    inputThread = std::thread(&StompClient::keyboardInputThread, this);
}

void StompClient::stop()
{
    running = false;
    if (commThread.joinable()) commThread.join();
    if (inputThread.joinable()) inputThread.join();
    socket.close();
}

void StompClient::communicationThread()
{
    try
	{
        while (running)
		{
            // Process server messages
            std::string message = readMessageFromServer();
            if (!message.empty()) {
                processServerMessage(message);
            }

            // Send messages from the queue
            std::unique_lock<std::mutex> lock(queueMutex);
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
        std::getline(std::cin, input);
        sendCommand(input);
    }
}
void StompClient::sendCommand(const std::string &command) {
    std::string frame;
    if (boost::algorithm::starts_with(command, "login ")) {
        frame = "CONNECT\naccept-version:1.2\nhost:localhost\nlogin:user\npasscode:1234\n\n\0";
    } else if (boost::algorithm::starts_with(command, "join ")) {
        std::string channel = command.substr(5);
        frame = "SUBSCRIBE\ndestination:/" + channel + "\nid:1\n\n\0";
    } else if (boost::algorithm::starts_with(command, "exit ")) {
        frame = "UNSUBSCRIBE\nid:1\n\n\0";
    } else if (boost::algorithm::starts_with(command, "send ")) {
        std::string message = command.substr(5);
        frame = "SEND\ndestination:/topic\n\n" + message + "\0";
    } else if (command == "logout") {
        frame = "DISCONNECT\nreceipt:123\n\n\0";
        running = false;
    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        return;
    }
    std::lock_guard<std::mutex> lock(queueMutex);
    messageQueue.push(frame);
    queueCond.notify_one();
	}

void StompClient::sendToServer(const std::string &frame)
{
    boost::asio::write(socket, boost::asio::buffer(frame));
}

std::string StompClient::readMessageFromServer()
{
    boost::asio::streambuf buffer;
    boost::asio::read_until(socket, buffer, '\0'); // Read until null character
    std::istream is(&buffer);
    std::string message;
    std::getline(is, message, '\0'); // Extract the message
    return message;
}

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
        body = boost::trim_copy(body);
        storeMessage(destination, user, body);
    }
}
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
    for (const auto &message : userIt->second) {
        outFile << message << std::endl;
    }
    std::cout << "Summary written to " << filename << std::endl;
}
bool StompClient::isRunning() const
{
	return running;
}