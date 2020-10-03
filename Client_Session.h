#pragma once

#include <boost/asio/ip/tcp.hpp>
#include "Server_Session.h"
#include "Message.h"

using boost::asio::ip::tcp;

class Client_Session : Common_Session {
    tcp::socket socket_;
    std::string username;
    Message read_msg_;
    WriteMsgs write_queue_;

public:
    Client_Session(tcp::socket &socket, std::string username);

    tcp::socket& socket();


private:
    void do_read_size();    //reads the size of the entire message and starts the reading of the action

    void do_read_body();    //reads the message and decodes actions and data

    void do_write();        //writes the available messages from the queue to the socket

    void enqueue_msg(const Message& msg);
};