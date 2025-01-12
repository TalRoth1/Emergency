#include "StompClient.h"
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>


int main(int argc, char *argv[])
{
StompClient::StompClient(const std::string &host, int port)
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

void StompClient::keyboardInputThread()
{
    while (running) {
        std::string input;
        std::getline(std::cin, input);

        // Translate keyboard input to STOMP frames
        if (boost::algorithm::starts_with(input, "login "))
		{
            std::string frame = "CONNECT\naccept-version:1.2\nhost:localhost\nlogin:user\npasscode:1234\n\n\0";
            std::lock_guard<std::mutex> lock(queueMutex);
            messageQueue.push(frame);
            queueCond.notify_one();
        } else if (boost::algorithm::starts_with(input, "join "))
		{
            std::string channel = input.substr(5);
            std::string frame = "SUBSCRIBE\ndestination:/" + channel + "\nid:1\n\n\0";
            std::lock_guard<std::mutex> lock(queueMutex);
            messageQueue.push(frame);
            queueCond.notify_one();
        } else if (boost::algorithm::starts_with(input, "exit "))
		{
            std::string channel = input.substr(5);
            std::string frame = "UNSUBSCRIBE\nid:1\n\n\0";
            std::lock_guard<std::mutex> lock(queueMutex);
            messageQueue.push(frame);
            queueCond.notify_one();
        } else if (boost::algorithm::starts_with(input, "send "))
		{
            std::string message = input.substr(5);
            std::string frame = "SEND\ndestination:/topic\n\n" + message + "\0";
            std::lock_guard<std::mutex> lock(queueMutex);
            messageQueue.push(frame);
            queueCond.notify_one();
        } else if (input == "logout") {
            std::string frame = "DISCONNECT\nreceipt:123\n\n\0";
            std::lock_guard<std::mutex> lock(queueMutex);
            messageQueue.push(frame);
            queueCond.notify_one();
            running = false; // Stop the loop
        } else {
            std::cerr << "Unknown command: " << input << std::endl;
        }
    }
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

void StompClient::processServerMessage(const std::string &message) {
    std::cout << "Server: " << message << std::endl;
}