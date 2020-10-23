#pragma once

#include <chrono>
#include <string>
#include <utility>
#include <boost/filesystem.hpp>
#include <thread>
#include <iostream>
#include <unordered_map>

enum class FileStatus {created, modified, erased};

struct RecPath {
    std::time_t lastEdit;
    bool isFile;
};

class DirectoryWatcher {
    bool running;
    std::string path_to_watch;
    std::unordered_map<std::string, RecPath> paths_;
    // Time interval at which we check the base folder for changes
    std::chrono::duration<int, std::milli> delay;

public:

    // Keep a record of files from the base directory and their last modification time
    DirectoryWatcher(std::string path_to_watch, std::chrono::duration<int, std::milli> delay, bool &running);

    // Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
    void start(const std::function<void (std::string, FileStatus, bool)> &action);
};