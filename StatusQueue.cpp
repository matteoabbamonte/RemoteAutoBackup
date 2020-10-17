#include "StatusQueue.h"

#include <utility>
#include <string>

StatusQueue::StatusQueue() : max_size(5) {}

void StatusQueue::push_status(int status, std::string data) {
    std::unique_lock ul(m);
    if (statQueue.size() >= max_size) {
        cv_full.wait(ul, [this](){return statQueue.size() < max_size;});
    }
    statQueue.push({status, std::move(data)});
}

status_data StatusQueue::pop_status() {
    std::unique_lock ul(m);
    if (statQueue.empty()) {
        cv_empty.wait(ul, [this](){return !statQueue.empty();});
    }
    auto res = statQueue.front();
    statQueue.pop();
    return res;
}


