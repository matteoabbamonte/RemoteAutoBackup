#pragma once


#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>

typedef struct Client_Action_Data {
    std::string username;
    std::string action;
    std::string data;
};

class OperationsQueue {
    std::queue<Client_Action_Data> opQueue;
    std::mutex m;
    std::condition_variable cv_full;
    std::condition_variable cv_empty;
    int max_size;

public:
    OperationsQueue(int size);

    void push_operation(std::string username, std::string action, std::string data);

    Client_Action_Data pop_operation();

};