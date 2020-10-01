#pragma once

#include <boost/asio/ip/tcp.hpp>
#include "Client.h"

using pClient = std::shared_ptr<Client>;
using boost::asio::ip::tcp;

class Backup_Server {
    tcp::acceptor acceptor;
    std::map<tcp::socket, pClient> clients;

    void do_accept();

public:
    Backup_Server(boost::asio::io_context& io_context, const tcp::endpoint& endpoint);
};