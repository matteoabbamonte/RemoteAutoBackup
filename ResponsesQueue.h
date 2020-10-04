#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

enum status_type
{
    ok = 0,
    created = 1,
    updated = 2,
    deleted = 3,
    no_need = 4,
    bad_request = 5,
    unauthorized = 6,
    in_need = 7,
    service_unavailable = 8
};

typedef struct status_data {
    status_type status;
    std::string data;
};

class ResponsesQueue {
    std::queue<status_data> resQueue;
    std::mutex m;
    std::condition_variable cv_full;
    std::condition_variable cv_empty;
    int max_size;

public:
    ResponsesQueue();

    void push_response(status_type status, std::string data);

    status_data pop_response();

};