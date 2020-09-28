#pragma once


#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>

class OperationsQueue {
    std::queue<std::tuple<std::string, std::string>> opQueue;
    std::mutex m;
    std::condition_variable cv_full;
    std::condition_variable cv_empty;
    int max_size;

public:
    OperationsQueue(int size);

    void push_operation(std::string action, std::string data);

    std::tuple<std::string, std::string> pop_operation();

};