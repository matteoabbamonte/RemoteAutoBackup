#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <deque>
#include <sqlite3.h>
#include "Message.h"
#include "OperationsQueue.h"
#include "Headers.h"
#include "Common_Session.h"

using boost::asio::ip::tcp;
using boost::property_tree::ptree;

class Server_Session {
    tcp::socket socket_;
    std::string username;
    Message read_msg_;
    std::map<std::string, std::size_t> paths;
    std::shared_ptr<Common_Session> commonSession;
    std::deque<Message> write_queue_s;

public:
    Server_Session(tcp::socket &socket);

    void start();

    //tcp::socket& socket();

    void insert_path(std::string path, size_t hash);

    void do_read_size();    //reads the size of the entire message and starts the reading of the action

    void do_read_body();    //reads the message and decodes actions and data

    void do_write();        //writes the available messages from the queue to the socket

    bool check_database(std::string username, std::string password);

    void enqueue_msg(const Message& msg);

    bool get_paths(std::string username);

    std::vector<std::string> compare_paths(ptree client_pt);
};
