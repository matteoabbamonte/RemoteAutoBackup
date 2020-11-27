#include "DirectoryWatcher.h"

DirectoryWatcher::DirectoryWatcher(std::string path_to_watch, boost::chrono::milliseconds delay, std::shared_ptr<bool> &watching)
        : path_to_watch(std::move(path_to_watch)), delay(delay), running_watcher(watching) {
    std::lock_guard lg(paths_mutex);
    for (boost::filesystem::directory_entry &element : boost::filesystem::recursive_directory_iterator(this->path_to_watch)) {
            auto lats_time_mod = boost::filesystem::last_write_time(element);
        paths[element.path().string()] = {lats_time_mod, boost::filesystem::is_regular_file(element), make_hash(element)};
    }
}

void DirectoryWatcher::start(const std::function<void (std::string, FileStatus, bool)>& action) {
    while (*running_watcher) {
        boost::this_thread::sleep_for(delay);
        std::lock_guard lg(paths_mutex);
        auto it = paths.begin();
        while (it != paths.end()) {
            if (!boost::filesystem::exists(it->first)) {
                action(it->first, FileStatus::erased, it->second.isFile);
                it = paths.erase(it);
            } else it++;
        }
        // Check if a file was created or modified
        for (boost::filesystem::directory_entry& element : boost::filesystem::recursive_directory_iterator(path_to_watch)) {
            auto lats_time_mod = boost::filesystem::last_write_time(element);
            if (paths.find(element.path().string()) == paths.end()) {                  /* Element creation */
                paths[element.path().string()] = {lats_time_mod, boost::filesystem::is_regular_file(element), make_hash(element)};
                action(element.path().string(), FileStatus::created, boost::filesystem::is_regular_file(element));
            } else if (paths[element.path().string()].lastEdit != lats_time_mod) {      /* Element modification */
                paths[element.path().string()] = {lats_time_mod, boost::filesystem::is_regular_file(element), make_hash(element)};
                action(element.path().string(), FileStatus::modified, boost::filesystem::is_regular_file(element));
            }
        }
    }
}

std::map<std::string, Node_Info> &DirectoryWatcher::getPaths() {
    std::lock_guard lg(paths_mutex);
    return paths;
}

Node_Info DirectoryWatcher::getNode(const std::string& path) {
    return paths[path];
}

size_t DirectoryWatcher::node_size(boost::filesystem::directory_entry& element) {
    int accum = 0;
    if (boost::filesystem::is_directory(element.path())) {
        for (boost::filesystem::directory_entry& sub_element : boost::filesystem::recursive_directory_iterator(element.path())) {
            accum += node_size(sub_element);
        }
    } else {
        accum = boost::filesystem::file_size(element);
    }
    return accum;
}

size_t DirectoryWatcher::make_hash(boost::filesystem::directory_entry& element) {
    auto lats_time_mod = boost::filesystem::last_write_time(element);
    std::hash<std::string> loc_hash;
    std::string loc_string = element.path().string() + std::to_string(lats_time_mod) + std::to_string(node_size(element));
    return loc_hash(loc_string);
}


