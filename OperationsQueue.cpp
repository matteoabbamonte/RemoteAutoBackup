#include "OperationsQueue.h"

OperationsQueue::OperationsQueue() : max_size(10) {}

void OperationsQueue::push_operation(std::string username, std::string header, std::string data) {
    std::unique_lock ul(m);
    if (opQueue.size() >= max_size) {
        cv_full.wait(ul, [this](){return opQueue.size() < max_size;});
    }
    opQueue.push({std::move(username), std::move(header), std::move(data)});
}

Client_Header_Data OperationsQueue::pop_operation() {
    std::unique_lock ul(m);
    if (opQueue.empty()) {
        cv_empty.wait(ul, [this](){return !opQueue.empty();});
    }
    auto op = opQueue.front();
    opQueue.pop();
    return op;
}
