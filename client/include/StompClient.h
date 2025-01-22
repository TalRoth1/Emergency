#ifndef STOMPCLIENT_H
#define STOMPCLIENT_H

#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <map>
#include <vector>
#include <fstream>
#include <atomic>
#include <boost/asio.hpp>

class StompClient {
public:
    // ** New default constructor (does NOT connect) **
    StompClient();

    // ** Deprecated or changed constructor (optional to keep it) **
    StompClient(const std::string &host, short port); 

    ~StompClient();

    // ** New method to actually connect the socket (moved out of the constructor). **
    void connectToServer(const std::string &host, short port);

    // Start the threads and send the CONNECT frame (with user+pass).
    void start(const std::string &username, const std::string &password);

    // Stop the client
    void stop();

    // Check if client is still running
    bool isRunning() const;

    // Command handling
    void sendCommand(const std::string &command);

    // Summaries, etc.
    void handleSummaryCommand(const std::string &channel, const std::string &user, const std::string &filename);

private:
    void communicationThread();
    void keyboardInputThread();

    // Helper to process server frames
    void processServerMessage(const std::string &frame);

    // Storing messages
    void storeMessage(const std::string &channel, const std::string &user, const std::string &message);

    // Low-level read/write
    void sendToServer(const std::string &frame);
    std::string readMessageFromServer();

    // Boost ASIO
    boost::asio::io_context ioContext;
    boost::asio::ip::tcp::socket socket;

    // Threads
    std::thread commThread;
    std::thread inputThread;

    // Thread-safe queue for sending frames to the server
    std::queue<std::string> messageQueue;
    std::mutex queueMutex;
    std::condition_variable queueCond;

    // Storage for messages
    std::map<std::string, std::map<std::string, std::vector<std::string>>> messages;
    std::mutex storageMutex;

    // Flag to indicate running
    std::atomic<bool> running;

    // ** Store the host, username, password if you want **
    std::string connectedHost;
    std::string username_;
    std::string password_;
};

#endif
