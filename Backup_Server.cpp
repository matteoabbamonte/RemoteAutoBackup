#include <iostream>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include "Server_Session.h"

using boost::asio::ip::tcp;

class Backup_Server {
    tcp::acceptor acceptor;

    void do_accept() {
        std::cout << "Waiting for incoming connections..." << std::endl;
        acceptor.async_accept(
                [this](boost::system::error_code ec, tcp::socket socket) {
                    if (!ec) {
                        std::make_shared<Server_Session>(socket)->start();
                    } else {
                        std::cerr << "Error inside do_accept: " << ec.message() << std::endl;
                    }
                    do_accept();
                });
    }

public:

    Backup_Server(boost::asio::io_context &io_context, const tcp::endpoint &endpoint) : acceptor(io_context, endpoint) {
        do_accept();
    };

};



int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cerr << "Usage: Backup_Server <port>\n";
        return 1;
    }

    boost::asio::io_context io_context;

    boost::asio::ip::tcp::resolver resolver(io_context);
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), std::atoi(argv[1]));
    Backup_Server bs(io_context, endpoint);

    io_context.run();

    return 0;
}