#include <iostream>
#include "DirectoryWatcher.h"
#include <boost/asio.hpp>
#include "Client.h"

int main(int argc, char* argv[]) {
    if (argc != 3)
    {
        std::cerr << "Usage: Client <host> <port>\n";
        return 1;
    }

    boost::asio::io_context io_context;

    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(argv[1], argv[2]);

    // Create a FileWatcher instance that will check the current folder for changes every 5 seconds
    DirectoryWatcher fw{"../../root", std::chrono::milliseconds(5000)};

    Client cl(io_context, endpoints);

    // Start monitoring a folder for changes and (in case of changes)
    // run a user provided lambda function
    fw.start(Client::send_actions);
}
