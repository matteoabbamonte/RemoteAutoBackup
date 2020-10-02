#pragma once

#include <boost/asio/ip/tcp.hpp>
#include "Client.h"

using pClient = std::shared_ptr<Client>;
using boost::asio::ip::tcp;

typedef struct pClient_username {
    pClient client_ptr;
    std::string username;
};

class Backup_Server {
    tcp::acceptor acceptor;
    std::map<tcp::socket, pClient_username> clients;

    void do_accept();

public:
    Backup_Server(boost::asio::io_context& io_context, const tcp::endpoint& endpoint);
};