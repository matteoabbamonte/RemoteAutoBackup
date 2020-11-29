#include "Backup_Server.h"

Backup_Server::Backup_Server(boost::asio::io_context &io_context, const tcp::endpoint &endpoint) : acceptor(io_context, endpoint) {
    do_accept();
};

void Backup_Server::do_accept() {
    std::cout << "Waiting for incoming connections..." << std::endl;
    acceptor.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
        if (!ec) {
            std::make_shared<Server_Session>(socket)->start();
        } else {
            std::cerr << "Error inside do_accept: " << ec.message() << std::endl;
        }
        do_accept();
    });
}