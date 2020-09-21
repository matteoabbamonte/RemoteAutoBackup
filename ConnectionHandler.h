//
// Created by matte on 21/09/2020.
//

#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>


class ConnectionHandler {
    boost::asio::ip::tcp::socket socket_;
    std::string message_;

    ConnectionHandler(boost::asio::io_context& io_context);
    void handle_write(const boost::system::error_code& /*error*/, size_t /*bytes_transferred*/);

public:
    typedef std::shared_ptr<ConnectionHandler> pointer;

    static pointer create(boost::asio::io_context& io_context);

    boost::asio::ip::tcp::socket& socket();


};
