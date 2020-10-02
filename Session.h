#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <deque>
#include "Message.h"
#include "OperationsQueue.h"
#include "Client.h"
#include <sqlite3>

using boost::asio::ip::tcp;
typedef std::deque<Message> WriteMsgs;
using pClient = std::shared_ptr<Client>;

typedef struct pClient_username {
    pClient client_ptr;
    std::string username;
};

class Common_Session {
    std::map<tcp::socket, pClient_username> clients;
    OperationsQueue operationsQueue;
public:
    void push(tcp::socket, const pClient_username& ptr_user);



    Common_Session();
};

class Session : Common_Session {
    boost::asio::ip::tcp::socket socket_;
    Message read_msg_;
    WriteMsgs write_queue_;

public:
    Session(tcp::socket socket);

    void start();

    boost::asio::ip::tcp::socket& socket();


private:
    void do_read_size();    //reads the size of the entire message and starts the reading of the action

    void do_read_body();    //reads the message and decodes actions and data

    void do_write();        //writes the available messages from the queue to the socket

    void enqueue_msg(const Message& msg);

    bool check_database(std::string username, std::string password);
};
