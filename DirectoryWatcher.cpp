#include "DirectoryWatcher.h"

DirectoryWatcher::DirectoryWatcher(std::string path_to_watch, boost::chrono::milliseconds delay, std::shared_ptr<bool> &watching)
        : path_to_watch(std::move(path_to_watch)), delay(delay), running_watcher(watching) {
    std::lock_guard lg(paths_mutex);    // Lock in order to guarantee thread safe access to the map
    for (boost::filesystem::directory_entry &element : boost::filesystem::recursive_directory_iterator(this->path_to_watch)) {  // Recursively iterating to path_to_watch
            auto last_time_edit = boost::filesystem::last_write_time(element);                                                  // in order to add the elements to the paths map
        paths[element.path().string()] = { last_time_edit, boost::filesystem::is_regular_file(element), make_hash(element) };
    }
}

void DirectoryWatcher::start(const std::function<void (std::string, FileStatus, bool)>& action) {
    while (*running_watcher) {      // Looping until the client session is closed
        boost::this_thread::sleep_for(delay);
        std::lock_guard lg(paths_mutex);     // Lock in order to guarantee thread safe access to the map
        auto it = paths.begin();
        while (it != paths.end()) {     // Looping checking the differences between the map and the local filesystem and
            if (!boost::filesystem::exists(it->first)) {    // If they're not aligned, the command to erase that specific node is sent to the server
                action(it->first, FileStatus::erased, it->second.isFile);
                it = paths.erase(it);
            } else it++;
        }
        try {
            for (boost::filesystem::directory_entry& element : boost::filesystem::recursive_directory_iterator(path_to_watch)) {     // Checking recursively if a file was created or modified
                auto last_time_edit = boost::filesystem::last_write_time(element);
                if (paths.find(element.path().string()) == paths.end()) {   // If the element is not present in the map, then it has been created
                    paths[element.path().string()] = { last_time_edit, boost::filesystem::is_regular_file(element), make_hash(element) };
                    action(element.path().string(), FileStatus::created, boost::filesystem::is_regular_file(element));      // The command to create that specific node is sent to the server
                } else if (paths[element.path().string()].lastEdit != last_time_edit) {      // Else if the element in the map has a different last_time_edit, then it has been updated
                    paths[element.path().string()] = { last_time_edit, boost::filesystem::is_regular_file(element), make_hash(element) };
                    action(element.path().string(), FileStatus::modified, boost::filesystem::is_regular_file(element));     // The command to modify that specific node is sent to the server
                }
            }
        } catch (const boost::filesystem::filesystem_error &err) {
            std::cout << "Element deleted before its insertion in the local map." << std::endl;
        }
    }
}

std::map<std::string, Node_Info> &DirectoryWatcher::getPaths() {
    std::lock_guard lg(paths_mutex);    // Lock in order to guarantee thread safe access to the map
    return paths;
}

Node_Info DirectoryWatcher::getNode(const std::string& path) {
    return paths[path];
}

size_t DirectoryWatcher::node_size(boost::filesystem::directory_entry& element) {
    int size = 0;
    if (boost::filesystem::is_directory(element.path())) {
        for (boost::filesystem::directory_entry& sub_element : boost::filesystem::recursive_directory_iterator(element.path())) {
            size += node_size(sub_element);     // Recursively calling node_size for each sub element of the directory
        }
    } else {
        size = boost::filesystem::file_size(element);
    }
    return size;
}

std::string DirectoryWatcher::make_hash(boost::filesystem::directory_entry& element) {
    unsigned char checksum[MD5_DIGEST_LENGTH];
    MD5_CTX md5;
    MD5_Init(&md5);
    if (!boost::filesystem::is_directory(element) && node_size(element) != 0) {
        std::ifstream file;
        file.open(element.path().string(), std::ios::in|std::ios::binary);  // Opening the file that has to be hashed
        file.seekg(0, std::ios::end);  // Finding the file length using seekg
        file.seekg(0, std::ios::beg);
        std::string buffer;
        std::string str;
        while (std::getline(file, str)) {
            if (!str.empty()) buffer.append(str);   // Reading from the file into a buffer vector
        }
        MD5_Update(&md5, buffer.data(), buffer.length());
    } else {
        auto last_time_edit = boost::filesystem::last_write_time(element);
        std::string info = element.path().string() + std::to_string(last_time_edit) + std::to_string(node_size(element));
        MD5_Update(&md5, info.data(), info.length());
    }
    MD5_Final(checksum, &md5);
    std::ostringstream sout;
    sout << std::hex << std::setfill('0');
    for (auto c: checksum) sout << std::setw(2) <<(int) c;
    return sout.str();
}
