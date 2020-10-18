#include <iostream>
#include <boost/asio.hpp>
#include "Backup_Server.h"

int main(int argc, char* argv[]) {
    if (argc < 3)
    {
        std::cerr << "Usage: Backup_Server <port>\n";
        return 1;
    }

    boost::asio::io_context io_context;

    boost::asio::ip::tcp::resolver resolver(io_context);
    boost::asio::ip::tcp::endpoint entpoint(boost::asio::ip::tcp::v4(), std::atoi(argv[1]));

    //auto endpoints = resolver.resolve(argv[1], argv[2]);
    io_context.run();

    return 0;
}