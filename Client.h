#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
//#include <boost/asio.hpp>
#include "Client_Session.h"

using boost::asio::ip::tcp;

class Client {
    boost::asio::io_context& io_context_;
    tcp::socket socket_;
    std::shared_ptr<Client_Session> cs;

    void do_connect(const tcp::resolver::results_type& endpoints);

public:
    Client(boost::asio::io_context& io_context, const tcp::resolver::results_type& endpoints);
};
