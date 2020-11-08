#include <iostream>
#include <boost/asio.hpp>
#include <deque>
#include <queue>
#include "DirectoryWatcher.h"
#include "Message.h"
#include "Headers.h"
#include "Base64/base64.h"

#define delimiter "\n}\n"

using boost::asio::ip::tcp;

class Client {
    boost::asio::io_context& io_context_;
    tcp::socket socket_;
    boost::asio::streambuf buf;
    std::shared_ptr<DirectoryWatcher> dw_ptr;
    std::queue<Message> write_queue_c;
    bool & running;

    void do_connect(const tcp::resolver::results_type& endpoints) {
        std::cout << "Trying to connect..." << std::endl;
        boost::asio::async_connect(socket_, endpoints,
                [this](boost::system::error_code ec, const tcp::endpoint&) {
                    std::cout << "Inside async_connect" << std::endl;
                    if (!ec) {
                        get_credentials();
                        do_read_body();
                        do_start_watcher();
                    } else {
                        std::cout << "Error while connecting: ";
                        std::cerr << ec.message() << std::endl;
                        socket_.close();
                        running = false;
                    }
                });
    }

    void do_start_watcher() {
        std::thread directoryWatcher([this](){
            dw_ptr->start([this](std::string path, FileStatus status, bool isFile) {
                // Process only regular files, all other file types are ignored
                if (boost::filesystem::is_regular_file(boost::filesystem::path(path))
                    || boost::filesystem::is_directory(boost::filesystem::path(path))
                    || status == FileStatus::erased) {
                    boost::property_tree::ptree pt;
                    int action_type;
                    std::string relative_path = path;
                    path = path.substr(path_to_watch.size() + 1);
                    if (path.find('.') < path.size())
                        path.replace(path.find('.'), 1, ":");

                    switch(status) {

                        case FileStatus::created: {

                            if (isFile)
                                std::cout << "File created: " << path << '\n';
                            else
                                std::cout << "Directory created: " << path << '\n';

                            std::ifstream inFile;
                            inFile.open(relative_path, std::ios::in|std::ios::binary);
                            std::vector<BYTE> buffer_vec;
                            char ch;
                            while (inFile.get(ch))
                                buffer_vec.emplace_back(ch);
                            std::string encodedData = base64_encode(&buffer_vec[0], buffer_vec.size());

                            pt.add("path", path);
                            pt.add("hash", DirectoryWatcher::paths_[relative_path].hash);
                            pt.add("isFile", isFile);
                            pt.add("content", encodedData);

                            action_type = 2;

                            break;
                        }

                        case FileStatus::modified : {

                            if (relative_path.find(':') < relative_path.size())
                                relative_path.replace(relative_path.find(':'), 1, ".");

                            pt.add("path", path);
                            pt.add("hash", DirectoryWatcher::paths_[relative_path].hash);
                            pt.add("isFile", isFile);

                            if (isFile) {
                                std::cout << "File modified: " << relative_path << '\n';

                                std::ifstream inFile;
                                inFile.open(relative_path, std::ios::in|std::ios::binary);
                                std::vector<BYTE> buffer_vec;
                                char ch;
                                while (inFile.get(ch))
                                    buffer_vec.emplace_back(ch);
                                std::string encodedData = base64_encode(&buffer_vec[0], buffer_vec.size());
                                pt.add("content", encodedData);

                            } else
                                std::cout << "Directory modified: " << relative_path << '\n';

                            action_type = 3;

                            break;

                        }

                        case FileStatus::erased : {

                            pt.add("path", path);
                            action_type = 4;

                            if (isFile)
                                std::cout << "File erased: " << path << '\n';
                            else
                                std::cout << "Directory erased: " << path << '\n';

                            break;
                        }

                        default:
                            std::cout << "Error! Unknown file status.\n";
                    }

                    //writing message
                    std::stringstream file_stream;
                    boost::property_tree::write_json(file_stream, pt, false);

                    std::string file_string(file_stream.str());

                    Message write_msg;
                    write_msg.encode_header(action_type);
                    write_msg.encode_data(file_string);
                    write_msg.zip_message();
                    enqueue_msg(write_msg);
                }
            });
        });
        directoryWatcher.detach();
    }

    void create_log_file() {
        boost::filesystem::ofstream("../../log.txt");
    }

    void do_read_body() {
        std::cout << "Reading message body..." << std::endl;
        boost::asio::async_read_until(socket_,
                                      buf,
                                      delimiter,
                                [this](boost::system::error_code ec, std::size_t length) {
                                    if (!ec) {
                                        std::string str(boost::asio::buffers_begin(buf.data()),
                                                        boost::asio::buffers_begin(buf.data()) + buf.size());
                                        buf.consume(length);
                                        Message msg;
                                        *msg.get_msg_ptr() = str;
                                        msg.get_msg_ptr()->resize(length);
                                        if (status_handler(msg)) {
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

    bool status_handler(Message msg) {
        msg.decode_message();
        auto status = static_cast<status_type>(msg.get_header()); //change header to status
        std::string data = msg.get_data();
        msg.clear();
        boost::filesystem::ofstream outFile;
        bool return_value = true;
        outFile.open("../../log.txt", std::ios::app);

        switch (status) {

            case status_type::in_need : {
                std::string separator = "||";
                size_t pos = 0;
                std::string path;
                while ((pos = data.find(separator)) != std::string::npos) {
                    path = data.substr(0, pos);
                    data.erase(0, pos + separator.length());
                    std::string relative_path = path;
                    if (relative_path.find(':') < relative_path.size())
                        relative_path.replace(relative_path.find(':'), 1, ".");
                    std::ifstream inFile;
                    relative_path = std::string(path_to_watch + "/") + relative_path;
                    inFile.open(relative_path, std::ios::in|std::ios::binary);
                    std::vector<BYTE> buffer_vec;
                    char ch;
                    while (inFile.get(ch))                  // loop getting single characters
                        buffer_vec.emplace_back(ch);
                    std::string encodedData = base64_encode(&buffer_vec[0], buffer_vec.size());
                    boost::property_tree::ptree pt;
                    pt.add("path", path);
                    pt.add("hash", DirectoryWatcher::paths_[relative_path].hash);
                    pt.add("isFile", DirectoryWatcher::paths_[relative_path].isFile);
                    pt.add("content", encodedData);

                    //writing message
                    std::stringstream file_stream;
                    boost::property_tree::write_json(file_stream, pt, false);

                    std::string file_string(file_stream.str());

                    Message write_msg;
                    write_msg.encode_header(2);
                    write_msg.encode_data(file_string);
                    write_msg.zip_message();
                    enqueue_msg(write_msg);
                }

                data = "Some paths needed";
                break;
            }

            case status_type::unauthorized : {
                std::cout << "Unauthorized." << std::endl;
                socket_.close();
                running = return_value = false;

                break;
            }

            case status_type::service_unavailable : {
                std::cout << "Service unavailable, shutting down." << std::endl;
                socket_.close();
                running = return_value = false;

                break;
            }

            case status_type::wrong_action : {
                std::cout << "Wrong action, rebooting." << std::endl;
                socket_.close();
                running = return_value = false;

                break;
            }

            case status_type::authorized : {
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

                break;
            }

            default : {
                std::cout << "Default status." << std::endl;

            }
        }

        data.append("\n");
        outFile.write(data.data(), data.size());
        outFile.close();
        return return_value;
    }

    void do_write() {
        std::cout << "Writing message..." << std::endl;
        boost::asio::async_write(socket_,
                boost::asio::dynamic_string_buffer(*write_queue_c.front().get_msg_ptr()),
                [this](boost::system::error_code ec, std::size_t /*length*/) {
                            if (!ec) {
                                write_queue_c.pop();
                                if (!write_queue_c.empty()) {
                                    do_write();
                                }
                            } else {
                                std::cout << "Error while writing: ";
                                std::cout << ec.message() << std::endl;
                                socket_.close();
                            }
                });
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
    std::string path_to_watch;

    Client(boost::asio::io_context& io_context, const tcp::resolver::results_type& endpoints, bool &running, std::string path_to_watch, DirectoryWatcher &dw) :
            io_context_(io_context),
            socket_(io_context),
            running(running),
            path_to_watch(path_to_watch),
            dw_ptr(std::shared_ptr<DirectoryWatcher>(&dw)) {
        do_connect(endpoints);
        create_log_file();
    }

    void close() {
        boost::asio::post(io_context_, [this]() {
            socket_.close();
            running = false;
        });
    }

    void enqueue_msg(const Message &msg) {
        bool write_in_progress = !write_queue_c.empty();
        write_queue_c.push(msg);
        if (!write_in_progress) do_write();
    }

    ~Client() {
        close();
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

        if (argc != 4) {
            std::cerr << "Usage: Client <host> <port> <rel_path_to_watch>\n";
            return 1;
        }

        std::string path_to_watch = argv[3];

        do {

            boost::asio::io_context io_context;

            boost::asio::ip::tcp::resolver resolver(io_context);
            auto endpoints = resolver.resolve(argv[1], argv[2]);

            bool running = true;

            // Create a FileWatcher instance that will check the current folder for changes every 5 seconds
            DirectoryWatcher dw{path_to_watch, std::chrono::milliseconds(5000), running};

            Client cl(io_context, endpoints, running, path_to_watch, dw);

            std::thread t([&io_context](){ io_context.run(); });

            t.join();

        } while (!stop());

    } catch (std::exception& e) {

        std::cerr << "Exception: " << e.what() << "\n";

    }
}




