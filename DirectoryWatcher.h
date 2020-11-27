#pragma once

#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <iostream>
#include <map>
#include "Headers.h"

//Struct for collecting info about files and directories
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

    //recursively calculating the size of a directory or a file
    size_t node_size(boost::filesystem::directory_entry& element);

    //calculating the hash of the node passed as input
    size_t make_hash(boost::filesystem::directory_entry& element);

public:

    //Keeping a record of files from the base directory and their info
    DirectoryWatcher(std::string path_to_watch, boost::chrono::milliseconds delay, std::shared_ptr<bool> &watching);

    //Monitoring "path_to_watch" for changes and in case of a change execute the user supplied "action" function
    void start(const std::function<void (std::string, FileStatus, bool)>& action);

    //getting the map containing the paths
    std::map<std::string, Node_Info>& getPaths();

    //getting the info about the single node given the path as input
    Node_Info getNode(const std::string& path);

};