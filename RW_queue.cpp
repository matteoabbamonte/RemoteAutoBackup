#include "RW_queue.h"

#include <utility>
#include <string>

RW_queue::RW_queue() {}

void RW_queue::push(Message msg) {
    std::unique_lock ul(m);
    cv.wait(ul, [this](){return messQueue.empty();});
    messQueue.push(std::move(msg));
}

Message RW_queue::pop() {
    std::unique_lock ul(m);
    if (messQueue.empty()) {
        cv.wait(ul, [this](){return !messQueue.empty();});
    }
    auto res = messQueue.front();
    messQueue.pop();
    return res;
}


