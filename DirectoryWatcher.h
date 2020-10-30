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
    std::size_t hash;
};

class DirectoryWatcher {
    bool & running;
    std::string path_to_watch;
    inline static std::unordered_map<std::string, RecPath> paths_;
    std::chrono::duration<int, std::milli> delay;

    //private methods

    size_t dirFile_Size(boost::filesystem::directory_entry& element);

    std::string get_path_to_watch();

    size_t make_hash(boost::filesystem::directory_entry& element);

    friend class Client;

public:

    // Keep a record of files from the base directory and their last modification time
    DirectoryWatcher(std::string path_to_watch, std::chrono::duration<int, std::milli> delay, bool &running);

    // Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
    void start(const std::function<void (std::string, FileStatus, bool)> &action);
};