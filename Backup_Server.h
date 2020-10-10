#pragma once

#include <boost/asio/ip/tcp.hpp>
#include "Client.h"
#include "Server_Session.h"

using boost::asio::ip::tcp;

class Backup_Server {
    tcp::acceptor acceptor;
    Common_Session* commonSession;

    void do_accept();

public:
    Backup_Server(boost::asio::io_context& io_context, const tcp::endpoint& endpoint);
};