#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <deque>
#include <sqlite3.h>
#include "Message.h"
#include "OperationsQueue.h"
#include "Client.h"
#include "Headers.h"
#include "Common_Session.h"

using boost::asio::ip::tcp;
typedef std::deque<Message> WriteMsgs;

class Server_Session : Common_Session {
    tcp::socket socket_;
    Message read_msg_;
    WriteMsgs write_queue_;
    std::map<std::string, std::string> paths;

public:
    Server_Session();

    Server_Session(tcp::socket &socket);

    void start();

    tcp::socket& socket();

    void do_read_size();    //reads the size of the entire message and starts the reading of the action

    void do_read_body();    //reads the message and decodes actions and data

    void do_write();        //writes the available messages from the queue to the socket

    void enqueue_msg(const Message& msg);

    bool check_database(std::string username, std::string password);

    bool get_paths(std::string username);
};
