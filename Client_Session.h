#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include "Message.h"
#include "StatusQueue.h"

using boost::asio::ip::tcp;

class Client_Session {
    StatusQueue statusQueue;
    tcp::socket socket_;
    std::string username;
    Message read_msg_;
    std::deque<Message> write_queue_c;

    void do_read_body();    //reads the message and decodes actions and data

    void do_write();        //writes the available messages from the queue to the socket

    void enqueue_msg(const Message& msg);

    void status_handler(int status, std::string data);

    void create_log_file();

public:
    Client_Session(tcp::socket &socket);

    void do_read_size();    //reads the size of the entire message and starts the reading of the action

    void get_credentials();

};