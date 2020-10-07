#include <iostream>
#include "DirectoryWatcher.h"

int main() {
    // Create a FileWatcher instance that will check the current folder for changes every 5 seconds
    DirectoryWatcher fw{"../../root", std::chrono::milliseconds(5000)};

    // Start monitoring a folder for changes and (in case of changes)
    // run a user provided lambda function
    fw.start([] (std::string path_to_watch, FileStatus status, bool isFile) -> void {
        // Process only regular files, all other file types are ignored
        if(boost::filesystem::is_regular_file(boost::filesystem::path(path_to_watch)) || boost::filesystem::is_directory(boost::filesystem::path(path_to_watch)) || status == FileStatus::erased) {
            switch(status) {
                case FileStatus::created:
                    if (isFile) {
                        std::cout << "File created: " << path_to_watch << '\n';
                    } else std::cout << "Directory created: " << path_to_watch << '\n';
                    break;
                case FileStatus::modified:
                    if (isFile) {
                        std::cout << "File modified: " << path_to_watch << '\n';
                    } else std::cout << "Directory modified: " << path_to_watch << '\n';
                    break;
                case FileStatus::erased:
                    if (isFile) {
                        std::cout << "File erased: " << path_to_watch << '\n';
                    } else std::cout << "Directory erased: " << path_to_watch << '\n';
                    break;
                default:
                    std::cout << "Error! Unknown file status.\n";
            }
        } else return;

    });
}
