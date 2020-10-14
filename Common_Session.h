#pragma once

#include <string>
#include <map>
#include "OperationsQueue.h"
#include "Client.h"
//#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class Common_Session {

    OperationsQueue operationsQueue;

public:
    Common_Session();

    OpInfo pop_op();

    void push_op(std::string username, int header, std::string data, tcp::socket &socket);

};
