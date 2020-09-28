//
// Created by matte on 21/09/2020.
//

#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "Message.h"

class Session {
    boost::asio::ip::tcp::socket socket_;
    Message read_msg_;

    Session(boost::asio::io_context& io_context);
    void handle_write(const boost::system::error_code& /*error*/, size_t /*bytes_transferred*/);

public:
    typedef std::shared_ptr<Session> pointer;

    static pointer create(boost::asio::io_context& io_context);

    boost::asio::ip::tcp::socket& socket();

private:
    void do_read_size();    //reads the size of the entire message and starts the reading of the action

    void do_read_body();    //reads the message and decodes actions and data

};
