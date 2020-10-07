#pragma once

#include <string>
#include <map>
#include "OperationsQueue.h"
#include "Client.h"

using pPaths = std::shared_ptr<std::map<std::string, std::string>>;
using boost::asio::ip::tcp;

typedef struct pPaths_username {
    pPaths paths_ptr;
    std::string username;
};

class Common_Session {
    std::map<tcp::socket, pPaths_username> clients;

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
