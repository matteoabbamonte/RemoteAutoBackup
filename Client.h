#pragma once

#include <boost/asio.hpp>
#include <boost/functional/hash.hpp>
#include <boost/timer/timer.hpp>
#include <iostream>
#include <queue>
#include "Base64/base64.h"
#include "DirectoryWatcher.h"
#include "Headers.h"
#include "Message.h"

using boost::asio::ip::tcp;

/// Struct for collecting the credentials related to a client
struct Credentials {
    std::string username;
    std::size_t password = 0;
};

class Client {
    boost::asio::io_context &io_context_;
    tcp::socket socket_;
    tcp::resolver::results_type endpoints;
    boost::asio::streambuf read_buf;
    std::shared_ptr<DirectoryWatcher> dw_ptr;
    std::queue<Message> write_queue_c;
    std::vector<std::string> paths_to_ignore;
    Credentials cred;
    boost::thread input_reader;
    boost::thread directory_watcher;
    std::string path_to_watch;
    int reconnection_counter = 0;
    boost::chrono::milliseconds delay;
    boost::timer::cpu_timer timer;
    std::shared_ptr<bool> running_client;
    std::shared_ptr<bool> running_watcher;
    std::shared_ptr<bool> stop;
    std::mutex input_mutex;
    std::mutex wq_mutex;
    std::condition_variable cv;

    /// Opens the connection with the server, calling the get_credentials and the do_read right after
    void do_connect();

    /// Reads the message from the socket and calls the appropriate handler
    void do_read();

    /// Writes the available messages from the queue to the socket
    void do_write();

    /// Adds messages to the write queue
    void enqueue_msg(const Message &msg);

    /// Starts the input_reader thread by calling the do_start_input_reader function, and waits for the credentials to be written
    void get_credentials();

    /// Sets the username in the Credentials structure
    void set_username(std::string &user);

    /// Sets the password in the Credentials structure
    void set_password(std::string &pwd);

    /// Creates the input_reader thread that manages all the user's input
    void do_start_input_reader();

    /// Creates the directory_watcher thread that loops over the path_to_watch
    void do_start_directory_watcher();

    /// Manages the errors occurred in the do_connect
    void handle_connection_failures();

    /// Manages the errors occurred in the do_read
    void handle_reading_failures();

    /// Manages the client status regarding the reconnection attempts frequency
    void handle_reconnection_timer();

    /// Manages the sending of the message containing all the local paths
    void handle_sync();

    /// Manages the decoding of the message and takes the needed actions
    void handle_status(Message msg);

    /// Encodes the content of the given file, and adds its info to the json that has to be sent
    void read_file(const std::string& path, const std::string& path_to_send, boost::property_tree::ptree& pt);

    /// Closes the socket client side, and waits for an answer to the reconnect attempt
    void close();

public:

    /// Starts the connection request with the server
    Client(boost::asio::io_context& io_context, tcp::resolver::results_type  endpoints,
           std::shared_ptr<bool> &running, std::string path_to_watch, std::shared_ptr<DirectoryWatcher> &dw, std::shared_ptr<bool> &stop, std::shared_ptr<bool> &watching);

    ~Client();
};
