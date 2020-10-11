#include <boost/filesystem.hpp>
#include "Common_Session.h"

Common_Session::Common_Session() {}

void Common_Session::push(tcp::socket &socket) {
    clients[socket] = "";
}

void Common_Session::push_username(tcp::socket &socket, const std::string username) {
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

OpInfo Common_Session::pop_op() {
    operationsQueue.pop_operation();
}

void Common_Session::push_op(std::string username, int header, std::string data, tcp::socket &socket) {
    operationsQueue.push_operation(username, header, data, socket);
}

Message Common_Session::front_wr() {
    return write_queue.front();
}

void Common_Session::pop_wr() {
    write_queue.pop_front();
}

bool Common_Session::empty_wr() {
    return write_queue.empty();
}

void Common_Session::enqueue_msg(const Message &msg) {
    write_queue.emplace_back(msg);
}

