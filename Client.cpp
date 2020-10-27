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
    bool & running;
    std::string path_to_watch;

    void do_connect(const tcp::resolver::results_type& endpoints) {
        std::cout << "Trying to connect..." << std::endl;
        boost::asio::async_connect(socket_, endpoints,
                [this](boost::system::error_code ec, const tcp::endpoint&) {
                    std::cout << "Inside async_connect" << std::endl;
                    if (!ec) {
                        get_credentials();
                        do_read_size();
                    } else {
                        std::cout << "Error while connecting: ";
                        std::cerr << ec.message() << std::endl;
                        socket_.close();
                        running = false;
                    }
                });
    }

    void create_log_file() {
        boost::filesystem::ofstream("../../log.txt");
    }

    void do_read_size() {
        std::cout << "Reading message size..." << std::endl;
        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_.get_size_ptr(), 10),
                                [this](boost::system::error_code ec, std::size_t /*length*/)
                                {
                                    if (!ec && read_msg_.decode_size())
                                    {
                                        do_read_body();
                                    }
                                    else
                                    {
                                        std::cout << "Error while reading message size: ";
                                        std::cout << ec.message() << std::endl;
                                        //socket_.close();
                                        running = false;
                                    }
                                });
    }

    void do_read_body() {
        std::cout << "Reading message body..." << std::endl;
        socket_.async_read_some(boost::asio::buffer(read_msg_.get_msg_ptr(read_msg_.get_size_int()), read_msg_.get_size_int()),
                                [this](boost::system::error_code ec, std::size_t /*length*/)
                                {
                                    if (!ec)
                                    {
                                        read_msg_.decode_message();
                                        //change header to status
                                        auto header = static_cast<status_type>(read_msg_.get_header());
                                        std::string data = read_msg_.get_data();
                                        //statusQueue.push_status(header, data);
                                        if (status_handler(header, data)) do_read_size();
                                    }
                                    else {
                                        std::cout << "Error while reding message body: ";
                                        std::cout << ec.message() << std::endl;
                                        socket_.close();
                                        running = false;
                                    }
                                });
    }

    bool status_handler(int status, std::string data) {
        boost::filesystem::ofstream outFile;
        bool return_value = true;
        outFile.open("../../log.txt", std::ios::app);
        data.append("\n");
        outFile.write(data.data(), data.size());
        outFile.close();
        switch (status) {
            case status_type::in_need:
            {
                // copiare il comando che faremo nello switch del main
                break;
            }
            case status_type::unauthorized:
            {
                std::cout << "Unauthorized." << std::endl;
                socket_.close();
                running = return_value = false;
                break;
            }
            case status_type::authorized:
            {
                std::cout << "Authorized." << std::endl;
                boost::property_tree::ptree pt;
                for (auto tuple : DirectoryWatcher::paths_) {
                    std::string path(tuple.first);
                    path = path.substr(path_to_watch.size()+1);
                    //path = path.substr(path.find('/')+1);
                    if (path.find('.') < path.size())
                        path.replace(path.find('.'), 1, ":");
                    pt.add(path, tuple.second.hash);
                }
                std::stringstream map_stream;
                std::string map_string;
                boost::property_tree::write_json(map_stream, pt);
                map_string = map_stream.str();

                Message write_msg;
                write_msg.encode_data(map_string);
                write_msg.encode_header(1);
                write_msg.zip_message();
                enqueue_msg(write_msg);
                break;
            }
            default:
            {
                std::cout << "Unrecognized status." << std::endl;
            }
        }
        return return_value;
    }

    void do_write() {
        std::cout << "Writing message..." << std::endl;
        boost::asio::async_write(socket_,
                                 boost::asio::buffer(write_queue_c.front().get_msg_ptr(),
                                                     write_queue_c.front().get_size_int()),
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
                                         std::cout << "Error while writing: ";
                                         std::cout << ec.message() << std::endl;
                                         socket_.close();
                                     }
                                 });
    }

    void enqueue_msg(const Message &msg) {
        write_queue_c.emplace_back(msg);
        do_write();
    }

    void get_credentials() {
        std::string username;
        std::cout << "Insert username: ";
        std::cin >> username;
        std::string password;
        std::cout << "Insert password: ";
        std::cin >> password;
        Message write_msg;
        write_msg.put_credentials(username, password);
        write_msg.encode_header(0);
        write_msg.zip_message();
        enqueue_msg(write_msg);
    }

public:
    Client(boost::asio::io_context& io_context, const tcp::resolver::results_type& endpoints, bool &running, std::string path_to_watch) : io_context_(io_context), socket_(io_context), running(running), path_to_watch(path_to_watch) {
        do_connect(endpoints);
        create_log_file();
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

    void close() {
        boost::asio::post(io_context_, [this]() {
            socket_.close();
            //running = false;
        });
    }
};

bool stop() {
    std::string stop;
    do {
        std::cout << "Do you want to reconnect? (y/n): ";
        std::cin >> stop;
    } while (stop != "y" && stop != "n");

    if (stop == "n") return true;
    return false;
}

int main(int argc, char* argv[]) {
    try {

        if (argc != 4)
        {
            std::cerr << "Usage: Client <host> <port> <rel_path_to_watch>\n";
            return 1;
        }

        do {

            boost::asio::io_context io_context;

            boost::asio::ip::tcp::resolver resolver(io_context);
            auto endpoints = resolver.resolve(argv[1], argv[2]);

            bool running = true;

            Client cl(io_context, endpoints, running, argv[3]);

            // Create a FileWatcher instance that will check the current folder for changes every 5 seconds
            DirectoryWatcher fw{argv[3], std::chrono::milliseconds(5000), running};

            std::thread t([&io_context](){ io_context.run(); });

            t.join();

            // Start monitoring a folder for changes and (in case of changes)
            // run a user provided lambda function
            fw.start(Client::send_actions);

        } while (!stop());

    } catch (std::exception& e) {

        std::cerr << "Exception: " << e.what() << "\n";

    }
}




