#pragma once
#include <iostream>
#include <boost/asio.hpp>
#include <queue>
#include "Base64/base64.h"
#include "DirectoryWatcher.h"
#include "Headers.h"
#include "Message.h"

using boost::asio::ip::tcp;

struct Credentials {
    std::string username;
    std::string password;
};

class Client {
    boost::asio::io_context &io_context_;
    tcp::socket socket_;
    tcp::resolver::results_type endpoints;
    boost::asio::streambuf buf;
    std::shared_ptr<DirectoryWatcher> dw_ptr;
    std::queue<Message> write_queue_c;
    Credentials cred;
    std::thread input_reader;
    std::thread directoryWatcher;
    std::string path_to_watch;
    std::chrono::duration<int, std::milli> delay;
    std::shared_ptr<bool> running;
    std::shared_ptr<bool> stop;
    std::shared_ptr<bool> watching;
    std::mutex m;
    std::condition_variable cv;

    void do_connect();

    void do_read_body();

    void do_write();

    void enqueue_msg(const Message &msg);

    void get_credentials();

    void set_username(std::string &user);

    void set_password(std::string &pwd);

    void do_start_input_reader();

    void do_start_watcher();

    void handle_connection_failures();

    void handle_reading_failures();

    void handle_synch();

    bool handle_status(Message msg);

    void close();

public:
    Client(boost::asio::io_context& io_context, const tcp::resolver::results_type& endpoints,
           std::shared_ptr<bool> &running, std::string path_to_watch, DirectoryWatcher &dw, std::shared_ptr<bool> &stop, std::shared_ptr<bool> &watching);

    ~Client();
};
