#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/exceptions.hpp>
#include <queue>
#include <sqlite3.h>
#include "Base64/base64.h"
#include "Database_Connection.h"
#include "Headers.h"
#include "Message.h"

#define delimiter "\n}\n"

using boost::asio::ip::tcp;
using boost::property_tree::ptree;

// Tracks the differences between the local paths map and the one sent by the client
struct Diff_paths {
    std::vector<std::string> toAdd;
    std::vector<std::string> toRem;
};

class Server_Session : public std::enable_shared_from_this<Server_Session> {
    tcp::socket socket_;
    std::string username;
    std::map<std::string, std::size_t> paths;
    std::queue<Message> write_queue_s;
    boost::asio::streambuf read_buf;
    std::mutex paths_mutex;
    std::mutex wq_mutex;
    Database_Connection db;

    // Reads the message from the socket and calls the appropriate handler
    void do_read_body();

    // Writes the available messages from the queue to the socket
    void do_write();

    // Adds messages to the write queue
    void enqueue_msg(const Message& msg);

    // Creates or updates file or directories received
    std::string do_write_element(action_type header, const std::string& data);

    // Deletes file or directories received
    void do_remove_element(const std::string& path);

    // Updates the paths map
    void update_paths(const std::string& path, size_t hash);

    // Compares the local map with the one sent by the client
    Diff_paths compare_paths(ptree &client_pt);

    // Decodes message and takes the needed actions
    void request_handler(Message msg);

public:

    Server_Session(tcp::socket &socket);

    // Calls for the first time the function that reads from the socket
    void start();

    ~Server_Session();

};
