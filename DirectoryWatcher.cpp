<<<<<<< HEAD
#include "DirectoryWatcher.h"

DirectoryWatcher::DirectoryWatcher(std::string path_to_watch, std::chrono::duration<int, std::milli> delay)
        : path_to_watch{std::move(path_to_watch)}, delay{delay} {
    for (boost::filesystem::directory_entry &element : boost::filesystem::recursive_directory_iterator(
            this->path_to_watch)) {
        if (element.path().filename().string().find(".",0) != 0)
            paths_[element.path().string()] = {boost::filesystem::last_write_time(element),
                                               boost::filesystem::is_regular_file(element)};
    }
}

void DirectoryWatcher::start(const std::function<void (std::string, FileStatus, bool)> &action) {
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
                auto current_file_last_write_time = boost::filesystem::last_write_time(element);
                //Element creation
                if (paths_.find(element.path().string()) == paths_.end()) {
                    paths_[element.path().string()] = {current_file_last_write_time, boost::filesystem::is_regular_file(element)};
                    action(element.path().string(), FileStatus::created, boost::filesystem::is_regular_file(element));
                    //Element modification
                } else if (paths_[element.path().string()].lastEdit != current_file_last_write_time) {
                    paths_[element.path().string()] = {current_file_last_write_time, boost::filesystem::is_regular_file(element)};
                    action(element.path().string(), FileStatus::modified, boost::filesystem::is_regular_file(element));
                }
            }
        }
    }
}
=======
#include "DirectoryWatcher.h"

DirectoryWatcher::DirectoryWatcher(std::string path_to_watch, std::chrono::duration<int, std::milli> delay)
        : path_to_watch{std::move(path_to_watch)}, delay{delay} {
    for (boost::filesystem::directory_entry &element : boost::filesystem::recursive_directory_iterator(
            this->path_to_watch)) {
        if (element.path().filename().string().find(".",0) != 0)
            paths_[element.path().string()] = {boost::filesystem::last_write_time(element),
                                               boost::filesystem::is_regular_file(element)};
    }
}

void DirectoryWatcher::start(const std::function<void (std::string, FileStatus, bool)> &action) {
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
                auto current_file_last_write_time = boost::filesystem::last_write_time(element);
                //Element creation
                if (paths_.find(element.path().string()) == paths_.end()) {
                    paths_[element.path().string()] = {current_file_last_write_time, boost::filesystem::is_regular_file(element)};
                    action(element.path().string(), FileStatus::created, boost::filesystem::is_regular_file(element));
                    //Element modification
                } else if (paths_[element.path().string()].lastEdit != current_file_last_write_time) {
                    paths_[element.path().string()] = {current_file_last_write_time, boost::filesystem::is_regular_file(element)};
                    action(element.path().string(), FileStatus::modified, boost::filesystem::is_regular_file(element));
                }
            }
        }
    }
}
>>>>>>> 69487d5e30893eed2f7e1d17ac4476279c170f24
