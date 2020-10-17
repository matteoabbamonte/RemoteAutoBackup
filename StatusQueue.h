#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include "Headers.h"

struct status_data {
    int status;
    std::string data;
};

class StatusQueue {
    std::queue<status_data> statQueue;
    std::mutex m;
    std::condition_variable cv_full;
    std::condition_variable cv_empty;
    int max_size;

public:
    StatusQueue();

    void push_status(int status, std::string data);

    status_data pop_status();

};