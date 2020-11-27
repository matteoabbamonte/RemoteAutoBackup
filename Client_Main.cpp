#include <iostream>
#include <boost/asio.hpp>
#include "Base64/base64.h"
#include "Client.h"
#include "DirectoryWatcher.h"


int main(int argc, char* argv[]) {

    try {

        if (argc != 4) {
            std::cerr << "Usage: Client <host> <port> <rel_path_to_watch>\n";
            return 1;
        }

        std::string path_to_watch = argv[3];
        auto stop = std::make_shared<bool>(false);

        do {
            auto running_client = std::make_shared<bool>(true);
            auto running_watcher = std::make_shared<bool>(true);
            boost::asio::io_context io_context;
            boost::asio::ip::tcp::resolver resolver(io_context);
            auto endpoints = resolver.resolve(argv[1], argv[2]);
            auto dw = std::make_shared<DirectoryWatcher>(path_to_watch, boost::chrono::milliseconds(500), running_watcher);
            Client cl(io_context, endpoints, running_client, path_to_watch, dw, stop, running_watcher);
            io_context.run();
        } while (!*stop);

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
