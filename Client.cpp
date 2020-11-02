#include <iostream>
#include <boost/asio.hpp>
#include <deque>
#include "DirectoryWatcher.h"
#include "Message.h"
#include "Headers.h"
#define delimiter "\n}\n"

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
                        do_read_body();
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

    void do_read_body() {
        std::cout << "Reading message body..." << std::endl;

        boost::asio::async_read_until(socket_,
                                      boost::asio::dynamic_string_buffer(*read_msg_.get_msg_ptr()),
                                      delimiter,
                                [this](boost::system::error_code ec, std::size_t /*length*/)
                                {
                                    if (!ec)
                                    {
                                        read_msg_.decode_message();
                                        //change header to status
                                        auto header = static_cast<status_type>(read_msg_.get_header());
                                        std::string data = read_msg_.get_data();
                                        if (status_handler(header, data)) {
                                            read_msg_.clear();
                                            do_read_body();
                                        }
                                    }
                                    else {
                                        std::cout << "Error while reading message body: ";
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
        switch (status) {
            case status_type::in_need:
            {
                std::string separator = "||";
                size_t pos = 0;
                std::string path;
                while ((pos = data.find(separator)) != std::string::npos) {
                    path = data.substr(0, pos);
                    std::string relative_path = path;
                    if (relative_path.find(':') < relative_path.size())
                        relative_path.replace(relative_path.find(':'), 1, ".");
                    std::ifstream inFile;
                    relative_path = std::string(path_to_watch + "/") + relative_path;
                    inFile.open(relative_path, std::ios::in|std::ios::binary);
                    std::vector<char> buffer_vec;
                    char buffer;
                    while (inFile.get(buffer))                  // loop getting single characters
                        buffer_vec.emplace_back(buffer);
                    std::string output(buffer_vec.begin(), buffer_vec.end());
                    data.erase(0, pos + separator.length());
                    boost::property_tree::ptree pt;
                    pt.add("path", path);
                    pt.add("hash", DirectoryWatcher::paths_[relative_path].hash);
                    pt.add("isFile", DirectoryWatcher::paths_[relative_path].isFile);
                    pt.add("content", output);

                    //writing message
                    std::stringstream file_stream;
                    boost::property_tree::write_json(file_stream, pt);
                    std::string file_string = file_stream.str();

                    Message write_msg;
                    write_msg.encode_data(file_string);
                    write_msg.encode_header(2);
                    write_msg.zip_message();
                    enqueue_msg(write_msg);
                }

                std::string log_txt("Some paths needed\n");
                outFile.write(log_txt.data(), log_txt.size());
                break;
            }
            case status_type::unauthorized:
            {
                std::cout << "Unauthorized." << std::endl;
                socket_.close();
                running = return_value = false;

                data.append("\n");
                outFile.write(data.data(), data.size());
                break;
            }
            case status_type::authorized:
            {
                std::cout << "Authorized." << std::endl;
                boost::property_tree::ptree pt;
                for (const auto& tuple : DirectoryWatcher::paths_) {
                    std::string path(tuple.first);
                    path = path.substr(path_to_watch.size()+1);
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

                data.append("\n");
                outFile.write(data.data(), data.size());
                break;
            }
            default:
            {
                std::cout << "Default status." << std::endl;
                data.append("\n");
                outFile.write(data.data(), data.size());
            }
        }
        outFile.close();
        return return_value;
    }

    void do_write() {
        std::cout << "Writing message..." << std::endl;
        boost::asio::async_write(socket_,
                                 boost::asio::dynamic_string_buffer(*write_queue_c.front().get_msg_ptr()),
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




