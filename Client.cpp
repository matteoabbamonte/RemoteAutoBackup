#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "DirectoryWatcher.h"
#include "Client_Session.h"

using boost::asio::ip::tcp;

class Client {
    boost::asio::io_context& io_context_;
    tcp::socket socket_;
    std::shared_ptr<Client_Session> cs;

    void do_connect(const tcp::resolver::results_type& endpoints) {
        boost::asio::async_connect(socket_, endpoints,
                [this](boost::system::error_code ec, const tcp::endpoint&) {
                    if (!ec) {
                        cs->do_read_size();
                    }
                });
    }

public:
    Client(boost::asio::io_context& io_context, const tcp::resolver::results_type& endpoints) : io_context_(io_context), socket_(io_context), cs(new Client_Session(socket_)) {
        do_connect(endpoints);
        cs->get_credentials();
    }

    static void send_actions(std::string path_to_watch, FileStatus status, bool isFile) {
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

    }
};


int main(int argc, char* argv[]) {
    if (argc != 3)
    {
        std::cerr << "Usage: Client <host> <port>\n";
        return 1;
    }

    boost::asio::io_context io_context;

    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(argv[1], argv[2]);

    // Create a FileWatcher instance that will check the current folder for changes every 5 seconds
    DirectoryWatcher fw{"../../root", std::chrono::milliseconds(5000)};

    Client cl(io_context, endpoints);

    // Start monitoring a folder for changes and (in case of changes)
    // run a user provided lambda function
    fw.start(Client::send_actions);
}




