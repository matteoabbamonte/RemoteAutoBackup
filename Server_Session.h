#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <deque>
#include <sqlite3.h>
#include "Message.h"
#include "Headers.h"
#include <boost/filesystem.hpp>
#include <queue>

#define delimiter "\n}\n"

using boost::asio::ip::tcp;
using boost::property_tree::ptree;

class Server_Session : public std::enable_shared_from_this<Server_Session> {
    tcp::socket socket_;
    std::string username;
    std::map<std::string, std::size_t> paths;
    std::queue<Message> write_queue_s;
    Message read_msg;

    void request_handler(Message msg, size_t length);

public:

    bool server_availability;

    Server_Session(tcp::socket &socket);

    void start();

    void update_paths(const std::string& path, size_t hash);

    void remove_path(const std::string& path);

    void do_read_body();    //reads the message and decodes actions and data

    void do_write();        //writes the available messages from the queue to the socket

    ~Server_Session();

    bool check_database(const std::string& username, const std::string& password);

    void enqueue_msg(const Message& msg, bool close);

    void update_db_paths();

    bool get_paths();

    std::vector<std::string> compare_paths(ptree client_pt);

};
