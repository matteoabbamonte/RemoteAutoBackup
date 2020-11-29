#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include "Server_Session.h"

using boost::asio::ip::tcp;

class Backup_Server {
    tcp::acceptor acceptor;

    /// Waits for and accepts incoming client connections
    void do_accept();

public:
    Backup_Server(boost::asio::io_context &io_context, const tcp::endpoint &endpoint);
};
