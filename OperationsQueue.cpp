#include "OperationsQueue.h"

#include <utility>

OperationsQueue::OperationsQueue(int size) : max_size(size) {}

void OperationsQueue::push_operation(std::string username, std::string action, std::string data) {
    std::unique_lock ul(m);
    if (opQueue.size() >= max_size) {
        cv_full.wait(ul, [this](){return opQueue.size() < max_size;});
    }
    opQueue.push({std::move(username), std::move(action), std::move(data)});
}

Client_Action_Data OperationsQueue::pop_operation() {
    std::unique_lock ul(m);
    if (opQueue.empty()) {
        cv_empty.wait(ul, [this](){return !opQueue.empty();});
    }
    auto op = opQueue.front();
    opQueue.pop();
    return op;
}
