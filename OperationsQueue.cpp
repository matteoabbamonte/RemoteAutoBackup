#include "OperationsQueue.h"

OperationsQueue::OperationsQueue() : max_size(10) {}

void OperationsQueue::push_operation(std::string username, int header, std::string data, tcp::socket &socket) {
    std::unique_lock<std::mutex> ul(m);
    if (opQueue.size() >= max_size) {
        cv_full.wait(ul, [this](){return opQueue.size() < max_size;});
    }
    opQueue.push({std::move(username), std::move(header), std::move(data), socket});
}

OpInfo OperationsQueue::pop_operation() {
    std::unique_lock<std::mutex> ul(m);
    if (opQueue.empty()) {
        cv_empty.wait(ul, [this](){return !opQueue.empty();});
    }
    auto op = std::move(opQueue.front());
    opQueue.pop();
    return op;
}
