#include "Client.h"

#define delimiter "\n}\n"

Client::Client(boost::asio::io_context& io_context, const tcp::resolver::results_type& endpoints,
        std::shared_ptr<bool> &running, std::string path_to_watch, DirectoryWatcher &dw, std::shared_ptr<bool> &stop, std::shared_ptr<bool> &watching)
        : io_context_(io_context), socket_(io_context), endpoints(endpoints), running(running),
        path_to_watch(path_to_watch), dw_ptr(std::make_shared<DirectoryWatcher>(dw)), stop(stop), watching(watching), delay(5000) {
            do_connect();
}

void Client::do_connect() {
    std::cout << "Trying to connect..." << std::endl;
    boost::asio::async_connect(socket_, endpoints, [this](boost::system::error_code ec, const tcp::endpoint&) {
        if (!ec) {
            get_credentials();
            do_read_body();
        } else {
            handle_connection_failures();
        }
    });
}

void Client::do_read_body() {
    std::cout << "Reading message body..." << std::endl;
    boost::asio::async_read_until(socket_,buf, delimiter, [this](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
            std::string str(boost::asio::buffers_begin(buf.data()), boost::asio::buffers_begin(buf.data()) + buf.size());
            buf.consume(length);
            Message msg;
            *msg.get_msg_ptr() = str;
            msg.get_msg_ptr()->resize(length);
            if (handle_status(msg)) do_read_body();
        } else {
            *watching = false;
            if (*running) handle_reading_failures();
        }
    });
}

void Client::do_write() {
    std::cout << "Writing message..." << std::endl;
    std::string str(*write_queue_c.front().get_msg_ptr());
    boost::asio::async_write(socket_, boost::asio::dynamic_string_buffer(*write_queue_c.front().get_msg_ptr()),
                             [this](boost::system::error_code ec, std::size_t /*length*/) {
                                 if (!ec) {
                                     write_queue_c.pop();
                                     if (!write_queue_c.empty()) do_write();
                                 } else {
                                     *watching = false;
                                     std::cerr << "Error while writing. ";
                                     close();
                                 }
                             });
}

void Client::enqueue_msg(const Message &msg) {
    bool write_in_progress = !write_queue_c.empty();
    write_queue_c.push(msg);
    if (!write_in_progress) do_write();
}

void Client::get_credentials() {
    std::unique_lock ul(m);
    do_start_input_reader();
    cv.wait(ul);
    Message login_message;
    login_message.put_credentials(cred.username, cred.password);
    enqueue_msg(login_message);
}

void Client::set_username(std::string &user) {
    cred.username = user;
}

void Client::set_password(std::string &pwd) {
    cred.password = pwd;
}

void Client::do_start_input_reader() {
    input_reader = std::thread([&](){
        std::string input;
        bool user_done = false;
        bool cred_done = false;
        std::cout << "Insert username: ";
        while (std::cin >> input) {
            if (cred_done) {
                if (input == "exit") {
                    if (*running) close();
                } else if (input == "y") {
                    std::lock_guard lg(m);
                    *stop = false;
                    cv.notify_all();
                    break;
                } else if (input == "n") {
                    std::lock_guard lg(m);
                    *stop = true;
                    cv.notify_all();
                    break;
                } else {
                    std::cerr << "Do you want to reconnect? (y/n): ";
                }
            } else {
                std::lock_guard lg(m);
                if (!user_done) {
                    set_username(input);
                    user_done = true;
                    std::cout << "Insert password: ";
                } else {
                    set_password(input);
                    cred_done = true;
                    cv.notify_all();
                }
            }
        }
    });
}

void Client::do_start_watcher() {
    if (!*watching) *watching = true;
    directoryWatcher = std::thread([this](){
        dw_ptr->start([this](std::string path, FileStatus status, bool isFile) {
            if (boost::filesystem::is_regular_file(boost::filesystem::path(path))   // Process only regular files, all other file types are ignored
            || boost::filesystem::is_directory(boost::filesystem::path(path)) || status == FileStatus::erased) {
                boost::property_tree::ptree pt;
                int action_type;
                std::string relative_path = path;
                path = path.substr(path_to_watch.size() + 1);
                if (path.find('.') < path.size()) path.replace(path.find('.'), 1, ":");
                switch(status) {
                    case FileStatus::created : {
                        if (isFile) std::cout << "File created: " << path << '\n';
                        else std::cout << "Directory created: " << path << '\n';
                        read_file(relative_path, path, isFile, pt);
                        action_type = 2;
                        break;
                    }
                    case FileStatus::modified : {
                        if (relative_path.find(':') < relative_path.size())
                            relative_path.replace(relative_path.find(':'), 1, ".");
                        if (isFile) {
                            std::cout << "File modified: " << relative_path << '\n';
                            read_file(relative_path, path, isFile, pt);
                        } else {
                            std::cout << "Directory modified: " << relative_path << '\n';
                        }
                        action_type = 3;
                        break;
                    }
                    case FileStatus::erased : {
                        pt.add("path", path);
                        action_type = 4;
                        if (isFile) std::cout << "File erased: " << path << '\n';
                        else std::cout << "Directory erased: " << path << '\n';
                        break;
                    }
                    default :
                        std::cout << "Error! Unknown file status.\n";
                }
                //writing message
                std::stringstream file_stream;
                boost::property_tree::write_json(file_stream, pt, false);
                std::string file_string(file_stream.str());
                Message write_msg;
                write_msg.encode_message(action_type, file_string);
                enqueue_msg(write_msg);
            }
        });
    });
    directoryWatcher.detach();
}

void Client::handle_connection_failures() {
    boost::asio::async_connect(socket_, endpoints, [this](boost::system::error_code ec, const tcp::endpoint&) {
        if (!ec) {
            get_credentials();
            do_read_body();
        } else {
            auto wait = std::chrono::duration_cast<std::chrono::seconds>(delay);
            std::cout << "Server unavailable, retrying in " << wait.count() << " sec" << std::endl;
            std::this_thread::sleep_for(delay);
            if (wait.count() >= 20) {
                std::cerr << "Server unavailable. ";
                std::cerr << "Do you want to reconnect? (y/n): ";
                std::string input;
                while (std::cin >> input) {
                    if (input == "n") {
                        *stop = true;
                        break;
                    } else if (input == "y") {
                        break;
                    } else {
                        std::cerr << "Do you want to reconnect? (y/n): ";
                    }
                }
            } else {
                delay *= 2;
                handle_connection_failures();
            }
        }
    });
}

void Client::handle_reading_failures() {
    boost::asio::async_connect(socket_, endpoints, [this](boost::system::error_code ec, const tcp::endpoint&) {
        if (!ec) {
            do_read_body();
        } else {
            auto wait = std::chrono::duration_cast<std::chrono::seconds>(delay);
            std::cout << "Server unavailable, retrying in " << wait.count() << " sec" << std::endl;
            std::this_thread::sleep_for(delay);
            if (wait.count() >= 20) {
                std::cerr << "Server unavailable. ";
                close();
            } else {
                delay *= 2;
                handle_reading_failures();
            }
        }
    });
}

void Client::handle_synch() {
    boost::property_tree::ptree pt;
    for (const auto& tuple : DirectoryWatcher::paths_) {
        std::string path(tuple.first);
        path = path.substr(path_to_watch.size()+1);
        if (path.find('.') < path.size()) path.replace(path.find('.'), 1, ":");
        pt.add(path, tuple.second.hash);
    }
    std::stringstream map_stream;
    boost::property_tree::write_json(map_stream, pt);
    std::string map_string = map_stream.str();
    Message write_msg;
    write_msg.encode_message(1, map_string);
    enqueue_msg(write_msg);
}

bool Client::handle_status(Message msg) {
    msg.decode_message();
    auto status = static_cast<status_type>(msg.get_header()); //change header to status
    std::string data = msg.get_data();
    msg.clear();
    bool return_value = true;
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
                while (inFile.get(ch)) buffer_vec.emplace_back(ch);
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
                write_msg.encode_message(2, file_string);
                enqueue_msg(write_msg);
            }
            break;
        }
        case status_type::unauthorized : {
            std::cerr << "Unauthorized. ";
            return_value = false;
            close();
            break;
        }
        case status_type::service_unavailable : {
            auto wait = std::chrono::duration_cast<std::chrono::seconds>(delay);
            std::cout << "Database unavailable, retrying in " << wait.count() << " sec" << std::endl;
            std::this_thread::sleep_for(delay);
            Message last_message;
            if (data == "login") {
                last_message.put_credentials(cred.username, cred.password);
                enqueue_msg(last_message);
            } else {
                handle_synch();
            }
            if (wait.count() >= 20) {
                std::cerr << "Database unavailable. ";
                return_value = false;
                close();
            } else {
                delay *= 2;
            }
            break;
        }
        case status_type::wrong_action : {
            std::cerr << "Wrong action. ";
            close();
            return_value = false;
            break;
        }
        case status_type::authorized : {
            std::cout << "Authorized." << std::endl;
            do_start_watcher();
            handle_synch();
            break;
        }
        default : {
            std::cout << "Default status." << std::endl;
        }
    }
    return return_value;
}

void Client::close() {
    *running = *watching = false;
    boost::asio::post(io_context_, [this]() {
        if (socket_.is_open()) socket_.close();
        std::unique_lock ul(m);
        std::cerr << "Do you want to reconnect? (y/n): ";
        cv.wait(ul);
    });
}

void Client::read_file(const std::string& relative_path, const std::string& path, bool isFile, boost::property_tree::ptree& pt) {
    std::ifstream inFile;
    inFile.open(relative_path, std::ios::in|std::ios::binary);
    std::vector<BYTE> buffer_vec;
    char ch;
    while (inFile.get(ch)) buffer_vec.emplace_back(ch);
    std::string encodedData = base64_encode(&buffer_vec[0], buffer_vec.size());
    pt.add("path", path);
    pt.add("hash", DirectoryWatcher::paths_[relative_path].hash);
    pt.add("isFile", isFile);
    pt.add("content", encodedData);
}

Client::~Client() {
    if (input_reader.joinable()) input_reader.join();
    if (directoryWatcher.joinable()) directoryWatcher.join();
}
