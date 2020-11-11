#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <sqlite3.h>
#include <boost/filesystem.hpp>
#include <queue>
#include "Message.h"
#include "Headers.h"
#include "Database_Connection.h"

#define delimiter "\n}\n"

struct Diff_vect {
    std::vector<std::string> toAdd;
    std::vector<std::string> toRem;
};

using boost::asio::ip::tcp;
using boost::property_tree::ptree;

class Server_Session : public std::enable_shared_from_this<Server_Session> {
    tcp::socket socket_;
    std::string username;
    std::map<std::string, std::size_t> paths;
    std::queue<Message> write_queue_s;
    boost::asio::streambuf buf;
    Database_Connection db;

    void request_handler(Message msg);

    void do_remove(const std::string& path);

    void update_paths(const std::string& path, size_t hash);

    void do_read_body();    //reads the message and decodes actions and data

    void do_write();        //writes the available messages from the queue to the socket

    void enqueue_msg(const Message& msg);;

    Diff_vect compare_paths(ptree &client_pt);

public:

    Server_Session(tcp::socket &socket);

    void start();

    ~Server_Session();

};
