#pragma once

#include <chrono>
#include <string>
#include <utility>
#include <boost/filesystem.hpp>
#include <thread>
#include <iostream>
#include <unordered_map>
#include "Headers.h"

struct RecPath {
    std::time_t lastEdit;
    bool isFile;
    std::size_t hash;
};

class DirectoryWatcher {
    std::shared_ptr<bool> watching;
    std::string path_to_watch;
    std::chrono::duration<int, std::milli> delay;

    //private methods

    size_t dirFile_Size(boost::filesystem::directory_entry& element);


    size_t make_hash(boost::filesystem::directory_entry& element);

    //friend class Client;

public:
    inline static std::map<std::string, RecPath> paths_;

    // Keep a record of files from the base directory and their last modification time
    DirectoryWatcher(std::string path_to_watch, std::chrono::duration<int, std::milli> delay, std::shared_ptr<bool> &watching);

    // Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
    void start(std::function<void (std::string, FileStatus, bool)> action);

};