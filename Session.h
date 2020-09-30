#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <deque>
#include "Message.h"

typedef  std::deque<Message> WriteMsgs;

class Session {
    boost::asio::ip::tcp::socket socket_;
    Message read_msg_;
    WriteMsgs write_queue_;

    Session(boost::asio::io_context& io_context);
    void handle_write(const boost::system::error_code& /*error*/, size_t /*bytes_transferred*/);

public:
    typedef std::shared_ptr<Session> pointer;

    static pointer create(boost::asio::io_context& io_context);

    boost::asio::ip::tcp::socket& socket();

private:
    void do_read_size();    //reads the size of the entire message and starts the reading of the action

    void do_read_body();    //reads the message and decodes actions and data

    void do_write();        //writes the available messages from the queue to the socket

};
