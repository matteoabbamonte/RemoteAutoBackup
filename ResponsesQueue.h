#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include "Headers.h"

typedef struct status_data {
    int status;
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

    void push_response(int status, std::string data);

    status_data pop_response();

};