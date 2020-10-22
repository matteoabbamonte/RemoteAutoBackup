#include <iostream>
#include <boost/asio.hpp>
#include <deque>
#include "DirectoryWatcher.h"
#include "Message.h"
#include "Headers.h"

using boost::asio::ip::tcp;

class Client {
    boost::asio::io_context& io_context_;
    tcp::socket socket_;
    Message read_msg_;
    std::deque<Message> write_queue_c;
    std::string username;

    void do_connect(const tcp::resolver::results_type& endpoints) {
        std::cout << "Trying to connect" << std::endl;
        boost::asio::async_connect(socket_, endpoints,
                [this](boost::system::error_code ec, const tcp::endpoint&) {
                    std::cout << "Inside async_connect" << std::endl;
                    if (!ec) {
                        std::cout << "Inside if" << std::endl;
                        get_credentials();
                        do_read_size();
                    } else {
                        std::cerr << ec.message() << std::endl;
                    }
                });
    }

    void create_log_file() {
        boost::filesystem::ofstream("../../log.txt");
    }

    void do_read_size() {
        std::cout << "Inside do_read_size" << std::endl;
        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_.get_size_ptr(), 10),
                                [this](boost::system::error_code ec, std::size_t /*length*/)
                                {
                                    if (!ec && read_msg_.decode_size())
                                    {
                                        std::cout << "Inside if" << std::endl;
                                        do_read_body();
                                    }
                                    else
                                    {
                                        std::cout << "Size is not a number" << std::endl;
                                    }
                                });
    }

    void do_read_body() {
        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_.get_msg_ptr(), read_msg_.get_size_int()),
                                [this](boost::system::error_code ec, std::size_t /*length*/)
                                {
                                    if (!ec)
                                    {
                                        read_msg_.decode_message();
                                        //change header to status
                                        auto header = static_cast<status_type>(read_msg_.get_header());
                                        std::string data = read_msg_.get_data();
                                        //statusQueue.push_status(header, data);
                                        status_handler(header, data);
                                        do_read_size();
                                    }
                                    else {
                                        std::cout << ec.message() << std::endl;
                                    }
                                });
    }

    void status_handler(int status, std::string data) {
        boost::filesystem::ofstream outFile;
        outFile.open("../../log.txt", std::ios::app);
        data.append("\n");
        outFile.write(data.data(), data.size());
        outFile.close();
        if (status == status_type::in_need) {
            // copiare il comando che faremo nello switch del main
        }
    }

    void do_write() {
        std::cout << "sono passato dalla write" << std::endl;
        boost::asio::async_write(socket_,
                                 boost::asio::buffer(write_queue_c.front().get_msg_ptr(),
                                                     write_queue_c.front().get_size_int()+2),
                                 [this](boost::system::error_code ec, std::size_t /*length*/)
                                 {
                                     if (!ec)
                                     {
                                         write_queue_c.pop_front();
                                         if (!write_queue_c.empty())
                                         {
                                             do_write();
                                         }
                                     }
                                     else
                                     {
                                         std::cout << ec.message() << std::endl;
                                     }
                                 });
    }

    void enqueue_msg(const Message &msg) {
        write_queue_c.emplace_back(msg);
        do_write();
    }

    void get_credentials() {
        std::cout << "Insert username: ";
        std::getline(std::cin, this->username);
        std::string password;
        std::cout << "Insert password: ";
        std::getline(std::cin, password);
        Message write_msg;
        write_msg.put_credentials(this->username, password);
        write_msg.encode_header(0);
        write_msg.zip_message();
        enqueue_msg(write_msg);
    }

public:
    Client(boost::asio::io_context& io_context, const tcp::resolver::results_type& endpoints) : io_context_(io_context), socket_(io_context) {
        do_connect(endpoints);
        //std::cout <<  << std::endl;

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

    std::thread t([&io_context](){ io_context.run(); });

    // Start monitoring a folder for changes and (in case of changes)
    // run a user provided lambda function
    fw.start(Client::send_actions);

    t.join();
}




