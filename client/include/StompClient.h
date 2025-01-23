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

private:
    void communicationThread();
    void keyboardInputThread();

    void sendToServer(const std::string &frame);

    // Read a raw STOMP frame from server (blocking)
    std::string readMessageFromServer();

    // Process the user commands from keyboard
    void sendCommand(const std::string &command);

    // Process frames from the server
    void processServerMessage(const std::string &frame);

    // Store a MESSAGE frame’s content for later summary
    void storeMessage(const std::string &channel,
                      const std::string &user,
                      const std::string &message);

    // For the “summary” command (local operation)
    void handleSummaryCommand(const std::string &channel,
                              const std::string &user,
                              const std::string &filename);

    // For the “report <file>” command
    void handleReportCommand(const std::string &file);

    // =========== Members =========== //

    // The IO Context and Socket
    boost::asio::io_context ioContext;
    boost::asio::ip::tcp::socket socket;

    // Thread-safety / concurrency
    std::mutex queueMutex;              // Protects messageQueue
    std::condition_variable queueCond;  // Notify communicationThread
    std::queue<std::string> messageQueue;

    std::mutex storageMutex; // Protects messages
    // messages[channel][user] => vector of strings
    std::map<std::string, std::map<std::string, std::vector<std::string>>> messages;

    std::thread commThread;
    std::thread inputThread;

    std::atomic<bool> running;

    // Keep track of the “current” username for forming events, etc.
    std::string currentUser;

    // Subscription IDs & receipt IDs
    int subIdCounter = 0;
    int receiptCounter = 0;

    // For each channel, which sub ID we used
    std::map<std::string, int> channelToSubId;

    // Stomp protocol says each SUBSCRIBE can have a unique sub ID
    // We also want unique receipts for certain frames.

};

#endif // STOMPCLIENT_H