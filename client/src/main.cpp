#include "../include/StompClient.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>

int main() {
    StompClient client;

    bool loggedIn = false;

    while (!loggedIn) {
        // Read the user's line
        std::string line;
        if (!std::getline(std::cin, line)) {
            // If EOF or error, exit
            return 1;
        }
        if (line.empty()) {
            continue; // skip empty lines
        }

        // Tokenize (split by whitespace)
        std::istringstream iss(line);
        std::vector<std::string> tokens;
        std::string token;
        short port; 
        while (iss >> token) {
            tokens.push_back(token);
        }

        // The very first valid command must be "login"
        if (tokens.empty() || tokens[0] != "login") {
            std::cerr << "first log in" << std::endl;
            continue;
        }

        // We expect 4 tokens: [login, host[:port], username, password]
        if (tokens.size() != 4) {
            std::cerr << "login need to contain {host} {username} {password}" << std::endl;
            continue;
        }

        // Extract them
        // tokens[0] == "login"
        // tokens[1] might be "127.0.0.1:7777" or just "127.0.0.1"
        std::string hostAndPort = tokens[1];
        std::string username = tokens[2];
        std::string password = tokens[3];

        // Default port if none was provided

        // Attempt to find a colon indicating "host:port"
        std::size_t colonPos = hostAndPort.find(':');
        if (colonPos != std::string::npos) {
            // Parse port from the substring after ":"
            try {
                port = static_cast<short>(std::stoi(hostAndPort.substr(colonPos + 1)));
            } catch (...) {
                std::cerr << "Invalid port in login command." << std::endl;
                continue;
            }

            // Host is up to (not including) the colon
            hostAndPort = hostAndPort.substr(0, colonPos);
        }

        // Now, hostAndPort should contain only "127.0.0.1"
        std::string host = hostAndPort;
        std::cout << host<< std::endl;
        std::cout << port<< std::endl;

        // Try to connect
        try {
            client.connectToServer(host, port);
            std::cout << "Connecting to " << host << ":" << port << "..." << std::endl;
            client.start(username, password);

            loggedIn = true;  // success

        } catch (const std::exception &ex) {
            std::cerr << "Error: " << ex.what() << std::endl;
            // Stay in loop to retry login
        }
    }

    // If we get here, the user is logged in, and threads are running
    // Remain until the client stops (e.g. user typed "logout")
    while (client.isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    client.stop();
    std::cout << "Client has stopped." << std::endl;
    return 0;
}
