#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include "Message.h"

class RW_queue {
    std::queue<Message> messQueue;
    std::mutex m;
    std::condition_variable cv;

public:
    RW_queue();

    void push(Message msg);

    Message pop();

};