#include "Common_Session.h"

Common_Session::Common_Session() {}

void Common_Session::push(tcp::socket &socket) {
    clients[socket] = "";
}

void Common_Session::push_username (tcp::socket &socket, const std::string username) {
    auto ptr_user = clients.find(socket);
    if (ptr_user != clients.end()) {
        clients[socket] = username;
    }
}

void Common_Session::delete_client(tcp::socket &socket) {
    clients.erase(socket);
}

std::string Common_Session::get_username(tcp::socket &socket) {
    return clients[socket];
}

Client_Header_Data Common_Session::pop_op() {
    operationsQueue.pop_operation();
}

