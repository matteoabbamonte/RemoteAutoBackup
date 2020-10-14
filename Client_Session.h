#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include "Message.h"
#include "ResponsesQueue.h"

using boost::asio::ip::tcp;

class Client_Session {
    ResponsesQueue responsesQueue;
    tcp::socket socket_;
    std::string username;
    Message read_msg_;
    std::deque<Message> write_queue_c;

public:
    Client_Session(tcp::socket &socket);

    tcp::socket& socket();

    void do_read_size();    //reads the size of the entire message and starts the reading of the action

    void get_credentials();

private:

    void do_read_body();    //reads the message and decodes actions and data

    void do_write();        //writes the available messages from the queue to the socket

    void enqueue_msg(const Message& msg);
};