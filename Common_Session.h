#pragma once

#include <string>
#include <map>
#include "Client.h"
#include "OperationsQueue.h"

using pClient = std::shared_ptr<Client>;
using boost::asio::ip::tcp;

typedef struct pClient_username {
    pClient client_ptr;
    std::string username;
};

class Common_Session {
    std::map<tcp::socket, std::string> clients;

protected:
    OperationsQueue operationsQueue;

public:
    Common_Session();

    void push(tcp::socket &socket);

    void push_username (tcp::socket &socket, const std::string username);

    std::string get_username(tcp::socket &socket);

    void delete_client(tcp::socket &socket);

    Client_Header_Data pop_op();

};
