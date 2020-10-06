#pragma once

#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>

typedef struct Client_Header_Data {
    std::string username;
    int header;
    std::string data;
};

class OperationsQueue {
    std::queue<Client_Header_Data> opQueue;
    std::mutex m;
    std::condition_variable cv_full;
    std::condition_variable cv_empty;
    int max_size;

public:
    OperationsQueue();

    void push_operation(std::string username, int header, std::string data);

    Client_Header_Data pop_operation();

};