#include <iostream>
#include <boost/asio.hpp>
#include "Backup_Server.h"


int main(int argc, char* argv[]) {

    try {

        if (argc < 2) {
            std::cerr << "Usage: Backup_Server <port>\n";
            return 1;
        }

        boost::asio::io_context io_context;
        boost::asio::ip::tcp::resolver resolver(io_context);
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), std::stoi(argv[1]));
        Backup_Server bs(io_context, endpoint);
        io_context.run();

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;

}