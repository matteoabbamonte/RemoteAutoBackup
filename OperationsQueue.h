#pragma once

#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>
#include <boost/asio/ip/tcp.hpp>

using boost::asio::ip::tcp;

struct OpInfo {
    std::string username;
    int header;
    std::string data;
    tcp::socket& socket;
};

class OperationsQueue {
    std::queue<OpInfo> opQueue;
    std::mutex m;
    std::condition_variable cv_full;
    std::condition_variable cv_empty;
    int max_size;

public:
    OperationsQueue();

    void push_operation(std::string username, int header, std::string data, tcp::socket &socket);

    OpInfo pop_operation();

};