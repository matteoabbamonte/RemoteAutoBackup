#include "DirectoryWatcher.h"

DirectoryWatcher::DirectoryWatcher(std::string path_to_watch, std::chrono::duration<int, std::milli> delay, std::shared_ptr<bool> &running)
        : path_to_watch{std::move(path_to_watch)}, delay{delay}, running(running) {
    for (boost::filesystem::directory_entry &element : boost::filesystem::recursive_directory_iterator(this->path_to_watch)) {
        if (element.path().filename().string().find(".",0) != 0) {
            auto lats_time_mod = boost::filesystem::last_write_time(element);
            paths_[element.path().string()] = {lats_time_mod, boost::filesystem::is_regular_file(element), make_hash(element)};
        }
    }
}

size_t DirectoryWatcher::dirFile_Size(boost::filesystem::directory_entry& element) {
    int accum = 0;
    if (boost::filesystem::is_directory(element.path())) {
        for (boost::filesystem::directory_entry& sub_element : boost::filesystem::recursive_directory_iterator(element.path())) {
            accum += dirFile_Size(sub_element);
        }
    } else {
        accum = boost::filesystem::file_size(element);
    }
    return accum;
}

void DirectoryWatcher::start(std::function<void (std::string, FileStatus, bool)> action) {
    while(running) {
        // Wait for "delay" milliseconds
        std::this_thread::sleep_for(delay);

        auto it = paths_.begin();
        while (it != paths_.end()) {
            if (!boost::filesystem::exists(it->first)) {
                action(it->first, FileStatus::erased, it->second.isFile);
                it = paths_.erase(it);
            } else it++;
        }

        // Check if a file was created or modified
        for (boost::filesystem::directory_entry& element : boost::filesystem::recursive_directory_iterator(path_to_watch)) {

            if (element.path().filename().string().find(".",0) != 0) {
                auto lats_time_mod = boost::filesystem::last_write_time(element);

                //Element creation
                if (paths_.find(element.path().string()) == paths_.end()) {
                    paths_[element.path().string()] = {lats_time_mod, boost::filesystem::is_regular_file(element), make_hash(element)};
                    action(element.path().string(), FileStatus::created, boost::filesystem::is_regular_file(element));
                    //Element modification
                } else if (paths_[element.path().string()].lastEdit != lats_time_mod) {
                    paths_[element.path().string()] = {lats_time_mod, boost::filesystem::is_regular_file(element), make_hash(element)};
                    action(element.path().string(), FileStatus::modified, boost::filesystem::is_regular_file(element));
                }
            }
        }
    }
}

size_t DirectoryWatcher::make_hash(boost::filesystem::directory_entry& element) {
    auto lats_time_mod = boost::filesystem::last_write_time(element);
    std::hash<std::string> loc_hash;
    std::string loc_string = element.path().string() + std::to_string(lats_time_mod) + std::to_string(dirFile_Size(element));
    return loc_hash(loc_string);
}


