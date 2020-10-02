#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <deque>
#include "Message.h"
#include "OperationsQueue.h"

typedef std::deque<Message> WriteMsgs;

class Session {
    boost::asio::ip::tcp::socket socket_;
    Message read_msg_;
    WriteMsgs write_queue_;

public:
    Session(boost::asio::io_context& io_context);

    void start (OperationsQueue & queue);

    boost::asio::ip::tcp::socket& socket();


private:
    void do_read_size(OperationsQueue & queue);    //reads the size of the entire message and starts the reading of the action

    void do_read_body(OperationsQueue & queue);    //reads the message and decodes actions and data

    void do_write();        //writes the available messages from the queue to the socket

    void enqueue_msg(const Message& msg);

    bool check_database(std::string username, std::string password);
};
