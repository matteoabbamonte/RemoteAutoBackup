#include "ResponsesQueue.h"

#include <utility>

ResponsesQueue::ResponsesQueue() : max_size(5) {}

void ResponsesQueue::push_response(status_type status, std::string data) {
    std::unique_lock ul(m);
    if (resQueue.size() >= max_size) {
        cv_full.wait(ul, [this](){return resQueue.size() < max_size;});
    }
    resQueue.push({status, std::move(data)});
}

status_data ResponsesQueue::pop_response() {
    std::unique_lock ul(m);
    if (resQueue.empty()) {
        cv_empty.wait(ul, [this](){return !resQueue.empty();});
    }
    auto res = resQueue.front();
    resQueue.pop();
    return res;
}


