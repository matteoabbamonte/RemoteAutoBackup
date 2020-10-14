#include <boost/filesystem.hpp>
#include "Common_Session.h"

Common_Session::Common_Session() {}

OpInfo Common_Session::pop_op() {
    return operationsQueue.pop_operation();
}

void Common_Session::push_op(std::string username, int header, std::string data, tcp::socket &socket) {
    operationsQueue.push_operation(username, header, data, socket);
}


