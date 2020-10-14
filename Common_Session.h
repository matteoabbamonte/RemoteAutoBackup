#pragma once

#include <string>
#include <map>
#include "OperationsQueue.h"
#include "Client.h"
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

/*typedef struct pPaths_username {
    pPaths paths_ptr;
    std::string username;
}; */

class Common_Session {
    std::map<tcp::socket,std::string> clients;
    OperationsQueue operationsQueue;

public:
    Common_Session();

    void push(tcp::socket &socket);

    void push_username(tcp::socket &socket, const std::string username);

    std::string get_username(tcp::socket &socket);

    void delete_client(tcp::socket &socket);

    OpInfo pop_op();

    void push_op(std::string username, int header, std::string data, tcp::socket &socket);

    virtual ~Common_Session();

};
