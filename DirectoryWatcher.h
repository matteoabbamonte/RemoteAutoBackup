#pragma once

#include <boost/chrono.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include <map>
#include <string>
#include "Headers.h"

// Struct for collecting information about files and directories
struct Node_Info {
    std::time_t lastEdit;
    bool isFile;
    std::size_t hash;
};

class DirectoryWatcher {
    std::shared_ptr<bool> running_watcher;
    std::string path_to_watch;
    std::mutex paths_mutex;
    boost::chrono::milliseconds delay;
    std::map<std::string, Node_Info> paths;

    // Recursively calculates the size of a directory or a file
    size_t node_size(boost::filesystem::directory_entry& element);

    // Calculates the hash of the node passed as input
    size_t make_hash(boost::filesystem::directory_entry& element);

public:

    // Keeps a record of files from the base directory and their info
    DirectoryWatcher(std::string path_to_watch, boost::chrono::milliseconds delay, std::shared_ptr<bool> &watching);

    // Monitors "path_to_watch" for changes and in case of a change execute the user supplied "action" function
    void start(const std::function<void (std::string, FileStatus, bool)>& action);

    // Gets the map containing the paths
    std::map<std::string, Node_Info>& getPaths();

    // Gets the info about the single node given the path as input
    Node_Info getNode(const std::string& path);

};