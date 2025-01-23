#include "../include/StompClient.h"
#include "../include/event.h"
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <sstream>
#include <chrono>

// =========== Constructors =========== //

StompClient::StompClient()
    : socket(ioContext),
      running(false),
      currentUser(""),
      subIdCounter(0),
      receiptCounter(0)
{}

StompClient::StompClient(const std::string &host, short port)
    : socket(ioContext),
      running(false),
      currentUser(""),
      subIdCounter(0),
      receiptCounter(0)      
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
    std::cout << "send frame connect: " << ss.str() << std::endl;

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
    try {
        socket.close();
    } catch (...) {
        // Ignore any exceptions
    }
}

// =========== Threads =========== //

void StompClient::communicationThread() {
    try {
        while (running) {
            // 1) Read messages from server
            std::string message = readMessageFromServer();
            if (!message.empty()) {
                processServerMessage(message);
                std::cout << "reading line: "<< message<< std::endl;

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
                std::cout << "sending to server frame that ready"<< frame <<std::endl;// never get here !!!
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
        if (!input.empty()) {
            sendCommand(input);
        }
    }
}

// =========== Command Handling =========== //
void StompClient::sendCommand(const std::string &command) {
    // We parse the command. Some commands are purely local (summary), 
    // some require building a STOMP frame.

    // Split by spaces for convenience
    std::istringstream iss(command);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    if (tokens.empty()) {
        return;
    }

    // Identify the primary command
    std::string primary = tokens[0];

    // === Join Command: "join <channel>" ===
    if (primary == "join" && tokens.size() == 2) {
        std::string channel = tokens[1];
        // create a new subscription ID
        
        std::cout << "get join command to channel : " << channel << std::endl;
        
        int subId = ++subIdCounter;
        channelToSubId[channel] = subId;
        int recId = ++receiptCounter;
        
        std::cout << "subId: " << subId << " recID: "<< recId <<std::endl;

        std::stringstream frame;
        frame << "SUBSCRIBE\n"
              << "destination:/" << channel << "\n"
              << "id:" << subId << "\n"
              << "receipt:" << recId << "\n"
              << "\n\0";

        // queue
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            messageQueue.push(frame.str());
            std::cout << "command join -> push frame to queue: " << frame.str() << std::endl;
            std::cout << "num of tasks: " << messageQueue.size() << std::endl;

            queueCond.notify_one();
        }

    // === Exit Command: "exit <channel>" ===
    } else if (primary == "exit" && tokens.size() == 2) {
        std::string channel = tokens[1];
        // find the subscription ID
        std:: cout << "command exit to channel : " << channel << std::endl;


        auto it = channelToSubId.find(channel);
        if (it == channelToSubId.end()) {
            std::cerr << "Not subscribed to channel " << channel << std::endl;
            return;
        }
        int subId = it->second;
        int recId = ++receiptCounter;

        std::cout << "subId: " << subId << "recID: "<< recId <<std::endl;

        std::stringstream frame;
        frame << "UNSUBSCRIBE\n"
              << "id:" << subId << "\n"
              << "receipt:" << recId << "\n"
              << "\n\0";

        // queue
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            messageQueue.push(frame.str());
            queueCond.notify_one();
        }
        // optionally remove from map
        channelToSubId.erase(channel);

    // === Report Command: "report <jsonFile>" ===
    } else if (primary == "report" && tokens.size() == 2) {
        std::string filePath = tokens[1];

        std::cout << "command report command to file : " << filePath << std::endl;

        handleReportCommand(filePath);

    // === Summary Command: "summary <channel> <user> <filename>" ===
    } else if (primary == "summary" && tokens.size() == 4) {
        std::string channel = tokens[1];
        std::string user = tokens[2];
        std::string filename = tokens[3];
        handleSummaryCommand(channel, user, filename);

        std::cout << "command summary command to channel : " << channel << " user: " << user << " filename: " << filename << std::endl;

    } else if (primary == "send") {
        // Just an example usage. 
        // Usually you'd do "send" to a *specific* channel if your assignment needs it.
        std::string textBody = command.substr(5); // everything after "send "
        // Potentially you can require a second token for channel, etc.
        // For simplicity, we do:
        int recId = ++receiptCounter;
        std::stringstream frame;
        frame << "SEND\n"
              << "destination:/topic" << "\n"
              << "receipt:" << recId << "\n"
              << "\n"
              << textBody << "\0";

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            messageQueue.push(frame.str()); //why push string not frame?? 
            std::cout << "send command -> push frame to queue: " << frame.str() << std::endl;
            std::cout << "num of tasks: " << messageQueue.size() << std::endl;

            queueCond.notify_one();
        }

    // === Logout Command ===
    } else if (primary == "logout") {
        // send DISCONNECT
        int recId = ++receiptCounter;
        std::stringstream frame;
        frame << "DISCONNECT\n"
              << "receipt:" << recId << "\n"
              << "\n\0";

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            messageQueue.push(frame.str());
            queueCond.notify_one();
        }

        // Mark to stop running after sending
        // The communication thread will see an empty queue next time, 
        // but let’s just set running = false after pushing.
        // We *could* wait for the RECEIPT, but that’s up to your design/spec.
        // Typically you'd wait to see the RECEIPT for a graceful shutdown.
        // We'll do the simplest approach:
        running = false;

    } else {
        std::cerr << "Unknown or invalid command: " << command << std::endl;
    }
}

// =========== “report” Implementation =========== //

void StompClient::handleReportCommand(const std::string &file) {
    // Example approach:
    // 1) parse the JSON for events (use parseEventsFile or something you wrote)
    // 2) For each event, build a SEND frame with all details
    // 3) Add frames to the queue

    // Suppose parseEventsFile(...) returns a vector of event objects
    // that contain {channel_name, city, event_name, date_time, description, general_info, etc.}

    names_and_events parsed = parseEventsFile(file);
    const std::string &channelName = parsed.channel_name;
    auto &eventsList = parsed.events;

    for (auto &ev : eventsList) {
        // Optionally store the “owner” user
        ev.setEventOwnerUser(currentUser);

        int recId = ++receiptCounter;

        // Build the body in STOMP “SEND” style
        std::stringstream body;
        // Body is textual lines, e.g.:
        body << "user:" << currentUser << "\n";
        body << "city:" << ev.get_city() << "\n";
        body << "event name:" << ev.get_name() << "\n";
        body << "date time:" << ev.get_date_time() << "\n";
        body << "general information:\n";
        for (auto &pair : ev.get_general_information()) {
            body << pair.first << ":" << pair.second << "\n";
        }
        body << "description:" << ev.get_description() << "\n";

        // Build STOMP frame
        std::stringstream frame;
        frame << "SEND\n"
              << "destination:/" << channelName << "\n"
              << "receipt:" << recId << "\n"
              << "\n"
              << body.str() 
              << "\0";

        // Queue the frame
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            messageQueue.push(frame.str());
            std::cout << "report command -> push frame to queue: " << frame.str() << std::endl;
            std::cout << "num of tasks: " << messageQueue.size() << std::endl;

        }
    }
    // Notify once, after pushing all frames
    queueCond.notify_one();
}

// =========== “summary” Implementation =========== //

void StompClient::handleSummaryCommand(const std::string &channel,
                                       const std::string &user,
                                       const std::string &filename)
{
    std::lock_guard<std::mutex> lock(storageMutex);

    // Check if we have stored anything for that channel
    auto channelIt = messages.find(channel);
    if (channelIt == messages.end()) {
        std::cout << "No messages for channel '" << channel << "'." << std::endl;
        return;
    }

    auto userIt = channelIt->second.find(user);
    if (userIt == channelIt->second.end()) {
        std::cout << "No messages for user '" << user << "' in channel '" << channel << "'." << std::endl;
        return;
    }

    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Could not open file " << filename << " for writing summary." << std::endl;
        return;
    }

    // Example simplified output:
    outFile << "Channel " << channel << " - Summary for user " << user << "\n";
    outFile << "----------------------------------------\n";
    for (const auto &msg : userIt->second) {
        outFile << msg << "\n";
    }
    outFile.close();

    std::cout << "Summary written to " << filename << std::endl;
}

// =========== Low-level Send/Receive =========== //

void StompClient::sendToServer(const std::string &frame) {
    boost::asio::write(socket, boost::asio::buffer(frame));
    std::cout << "sending  to server by writing to buffer:  " << frame << std::endl;
}

std::string StompClient::readMessageFromServer() {
    // The simplest approach is to read until '\0' each time
    // but keep in mind the server might send multiple frames in one chunk.
    // For a naive approach, we do one frame per read. 
    boost::asio::streambuf buffer;
    boost::system::error_code error;

    // If nothing is there to read, this might block. 
    // In real usage, you might set a timeout or do async reads.
    size_t bytes = boost::asio::read_until(socket, buffer, '\0', error);

    if (error && error != boost::asio::error::eof) {
        // If it’s eof, we can interpret that as server closed.
        // If it’s something else, handle it.
        return "";
    }

    std::istream is(&buffer);
    std::string message;
    std::getline(is, message, '\0');
    return message;
}

// =========== Process Incoming Frames from Server =========== //

void StompClient::processServerMessage(const std::string &frame) {
    // Very rough parsing:
    // The first line is the command: CONNECTED, MESSAGE, RECEIPT, ERROR, etc.
    std::istringstream iss(frame);
    std::string command;
    std::getline(iss, command);

    if (command == "CONNECTED") {
        std::cout << "Login successful" << std::endl;

    } else if (command == "MESSAGE") {
        // We expect lines like:
        //   destination:/someChannel
        //   user:bob
        //   [blank line]
        //   body...
        // Gather up headers + body
        std::string line;
        std::string destination, user;
        bool readingBody = false;
        std::string body;

        while (std::getline(iss, line)) {
            if (readingBody) {
                // Part of the body
                body += (line + "\n");
            } else {
                // Check for empty line => start body
                if (line.empty()) {
                    readingBody = true;
                    continue;
                }
                // Otherwise parse headers
                if (boost::algorithm::starts_with(line, "destination:")) {
                    destination = line.substr(std::string("destination:").size());
                } else if (boost::algorithm::starts_with(line, "user:")) {
                    user = line.substr(std::string("user:").size());
                }
            }
        }
        boost::algorithm::trim(destination);
        boost::algorithm::trim(user);
        boost::algorithm::trim(body);

        // In your assignment, the “destination” might not start with a slash, or might. 
        // You can store it as is, or remove the leading “/” if you like:
        if (!destination.empty() && destination[0] == '/') {
            destination.erase(0, 1);
        }

        // store
        storeMessage(destination, user, body);

    } else if (command == "RECEIPT") {
        // The server acknowledges a frame with "receipt-id:xxx"
        // We can parse the lines to see which receipt
        std::string line;
        std::string receiptId;
        while (std::getline(iss, line)) {
            if (boost::algorithm::starts_with(line, "receipt-id:")) {
                receiptId = line.substr(11);
                boost::algorithm::trim(receiptId);
            }
        }
        std::cout << "[DEBUG] Received RECEIPT for id: " << receiptId << std::endl;

    } else if (command == "ERROR") {
        // Possibly parse “message:” header or body
        std::string line;
        std::string errorMsg;
        while (std::getline(iss, line)) {
            errorMsg += line + "\n";
        }
        std::cerr << "[ERROR Frame from Server] " << errorMsg << std::endl;
        // Depending on your specs, you might stop.
        // running = false;
    }
}

// =========== Message Storage / Summary =========== //

void StompClient::storeMessage(const std::string &channel, const std::string &user, const std::string &message) {
    std::lock_guard<std::mutex> lock(storageMutex);
    messages[channel][user].push_back(message);
}


// =========== Status =========== //

bool StompClient::isRunning() const {
    return running;
}
