#include "../include/StompClient.h"
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    // Ensure proper usage
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <host> <port>" << std::endl;
        return 1;
    }

    // Parse host and port
    std::string host = argv[1];
    short port = std::stoi(argv[2]);

    try {
        // Create the StompClient and start it
        StompClient client(host, port);
        std::cout << "Connecting to " << host << ":" << port << "..." << std::endl;
        client.start();

        // Wait for user commands in the background
        std::cout << "Client is running. Enter commands:" << std::endl;
        while (client.isRunning()) {
            // Let the client handle commands and server communication
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Stop the client gracefully
        client.stop();
        std::cout << "Client has stopped." << std::endl;

    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
