#include "Client.h"

#define delimiter "\n}\n"

Client::Client(boost::asio::io_context& io_context, tcp::resolver::results_type  endpoints,
        std::shared_ptr<bool> &running_client, std::string path_to_watch, std::shared_ptr<DirectoryWatcher> &dw, std::shared_ptr<bool> &stop, std::shared_ptr<bool> &running_watcher)
        : io_context_(io_context), socket_(io_context), endpoints(std::move(endpoints)), running_client(running_client),
        path_to_watch(std::move(path_to_watch)), dw_ptr(dw), stop(stop), running_watcher(running_watcher), delay(5000) {
            do_connect();
}

void Client::do_connect() {
    std::cout << "Trying to connect..." << std::endl;
    boost::asio::async_connect(socket_, endpoints, [this](boost::system::error_code ec, const tcp::endpoint&) { //asynchronous connection request
        if (!ec) {
            get_credentials();
            do_read();
        } else {
            handle_connection_failures();
        }
    });
}

void Client::do_read() {
    std::cout << "Reading message..." << std::endl;
    boost::asio::async_read_until(socket_,read_buf, delimiter, [this](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
            std::string str(boost::asio::buffers_begin(read_buf.data()),
                            boost::asio::buffers_begin(read_buf.data()) + read_buf.size());
            read_buf.consume(length);     // Crop buffer in order to let the next do_read work properly
            str.resize(length);           // Crop in order to erase residuals taken from the buffer
            Message msg;
            *msg.get_msg_ptr() = str;
            handle_status(msg);
            do_read();
        } else {
            *running_watcher = false;   // Signaling to the directory watcher the end of the client session
            if (*running_client) handle_reading_failures();   // If the socket has been closed by the server then call the EOF handler
        }
    });
}

void Client::do_write() {
    std::cout << "Writing message..." << std::endl;
    boost::asio::async_write(socket_, boost::asio::dynamic_string_buffer(*write_queue_c.front().get_msg_ptr()),
            [this](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    std::lock_guard lg(wq_mutex);   // Lock in order to guarantee thread safe pop operation
                    write_queue_c.pop();
                    if (!write_queue_c.empty()) do_write();
                } else {
                    *running_watcher = false;   // Signaling to the directory watcher the end of the client session
                    std::cerr << "Error while writing. ";
                    close();
                }
    });
}

void Client::enqueue_msg(const Message &msg) {
    std::lock_guard lg(wq_mutex);   // Lock in order to guarantee thread safe push operation
    bool write_in_progress = !write_queue_c.empty();
    write_queue_c.push(msg);
    if (!write_in_progress) do_write();    // Call do_write only if it is not already running
}

void Client::get_credentials() {
    try {
        std::unique_lock ul(input_mutex);   // Unique lock in order to use the cv wait
        do_start_input_reader();
        cv.wait(ul, [this](){return !cred.username.empty() && !cred.password.empty();});    // Waiting for the input reader thread to receive the credentials
        Message login_message;
        login_message.put_credentials(cred.username, cred.password);    // Saving the credentials in the message that has to be sent
        enqueue_msg(login_message);
    } catch (const boost::property_tree::ptree_error &err) {
        std::cerr << "Error while completing login procedure. ";
        close();
    }
}

void Client::set_username(std::string &user) {
    cred.username = user;
}

void Client::set_password(std::string &pwd) {
    cred.password = pwd;
}

void Client::do_start_input_reader() {
    input_reader = boost::thread([this](){
        std::string input;
        bool user_done = false;    // True if the username has been inserted
        bool cred_done = false;    // True if the credentials have been inserted
        std::cout << "Insert username: ";
        while (std::cin >> input) {
            if (!std::cin) close();    // If there is any error during the input process then close.
            if (cred_done) {
                if (input == "exit") {
                    if (*running_client) close();   // If the input is equal to exit and the client session is still up then close
                    else std::cerr << "Do you want to reconnect? (y/n): ";
                } else if (input == "y") {     // If the input is equal to y then the thread sets the value of stop to false and notifies the input to the client thread
                    std::lock_guard lg(input_mutex);
                    *stop = false;
                    cv.notify_all();
                    break;
                } else if (input == "n") {     // If the input is equal to y then the thread sets the value of stop to true and notifies the input to the client thread
                    std::lock_guard lg(input_mutex);
                    *stop = true;
                    cv.notify_all();
                    break;
                } else if (!*running_client){   // If the input is not an expected input and the client is not running anymore then repeat the question
                    std::cerr << "Do you want to reconnect? (y/n): ";
                }
            } else {
                std::lock_guard lg(input_mutex);
                if (!user_done) {   // If the username has not been inserted then its set function is called on the input
                    set_username(input);
                    user_done = true;
                    std::cout << "Insert password: ";
                } else {    // else the password set function is called on the input
                    set_password(input);
                    cred_done = true;
                    cv.notify_all();    // Waking up the client thread in the get_credentials function
                }
            }
        }
    });
}

void Client::do_start_directory_watcher() {
    if (!*running_watcher) *running_watcher = true;
    directory_watcher = boost::thread([this](){
        dw_ptr->start([this](std::string path, FileStatus status, bool isFile) {
            if (boost::filesystem::is_regular_file(boost::filesystem::path(path))   // Process only regular files, all other file types are ignored
            || boost::filesystem::is_directory(boost::filesystem::path(path)) || status == FileStatus::erased) {
                boost::property_tree::ptree pt;
                int action_type = 999;
                std::string relative_path = path;
                path = path.substr(path_to_watch.size() + 1);
                while (path.find('.') < path.size()) path.replace(path.find('.'), 1, ":");
                switch (status) {
                    case FileStatus::created : {
                        if (isFile) std::cout << "File created: " << path << '\n';
                        else std::cout << "Directory created: " << path << '\n';
                        try {
                            read_file(relative_path, path, pt);
                            action_type = 2;
                        } catch (...) {
                            std::cerr << "Error while opening the file: " << path << " It won't be sent." << std::endl;
                            paths_to_ignore.emplace_back(path);
                        }
                        break;
                    }
                    case FileStatus::modified : {
                        while (relative_path.find(':') < relative_path.size())
                            relative_path.replace(relative_path.find(':'), 1, ".");
                        if (isFile) {
                            std::cout << "File modified: " << relative_path << '\n';
                            try {
                                read_file(relative_path, path, pt);
                                action_type = 3;
                            } catch (...) {
                                std::cerr << "Error while opening the file: " << path << "\nIt won't be sent." << std::endl;
                                paths_to_ignore.emplace_back(path);
                            }
                        } else {
                            std::cout << "Directory modified: " << relative_path << '\n';
                        }
                        break;
                    }
                    case FileStatus::erased : {
                        try {
                            if (std::find(paths_to_ignore.begin(), paths_to_ignore.end(), path) == paths_to_ignore.end()) {
                                pt.add("path", path);
                                action_type = 4;
                                if (isFile) std::cout << "File erased: " << path << '\n';
                                else std::cout << "Directory erased: " << path << '\n';
                            }
                        } catch (const boost::property_tree::ptree_error &err) {
                            std::cerr << "Error while executing the action on the file " << path << ", closing session. " << std::endl;
                            close();
                        }
                        break;
                    }
                    default :
                        std::cout << "Error! Unknown file status.\n";
                }
                //writing message
                if (action_type <= 4) {
                    try {
                        std::stringstream file_stream;
                        boost::property_tree::write_json(file_stream, pt, false);   // Saving the json in a stream
                        std::string file_string(file_stream.str());
                        Message write_msg;
                        write_msg.encode_message(action_type, file_string);
                        enqueue_msg(write_msg);
                    } catch (const boost::property_tree::ptree_error &err) {
                        paths_to_ignore.emplace_back(path);
                        std::cerr << "Error while executing the action on the file " << path << ", it won't be sent. " << std::endl;
                        std::cerr << "If you want to resynchronize write \'exit\'." << std::endl;
                    }
                }
            }
        });
    });
}

void Client::handle_connection_failures() {
    boost::asio::async_connect(socket_, endpoints, [this](boost::system::error_code ec, const tcp::endpoint&) {
        if (!ec) {
            delay = boost::chrono::milliseconds(5000);  // Resetting delay to the initial value
            get_credentials();
            do_read();
        } else {
            auto wait = boost::chrono::milliseconds(delay);
            std::cout << "Server unavailable, retrying in " << wait.count() << " sec" << std::endl;
            boost::this_thread::sleep_for(delay);
            if (wait.count() >= 20) {
                std::cerr << "Server unavailable. ";
                std::cerr << "Do you want to reconnect? (y/n): ";
                std::string input;
                while (std::cin >> input) {
                    if (!std::cin) close();
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
            try {
                delay = boost::chrono::milliseconds(5000);  // Resetting delay to the initial value
                Message login_message;
                login_message.put_credentials(cred.username, cred.password);
                enqueue_msg(login_message);
                do_start_directory_watcher();
                do_read();
            } catch (const boost::property_tree::ptree_error &err) {
                std::cerr << "Error while reconnecting. ";
                close();
            }
        } else {
            auto wait = boost::chrono::milliseconds(delay);
            std::cout << "Server unavailable, retrying in " << wait.count() << " sec" << std::endl;
            boost::this_thread::sleep_for(delay);
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

void Client::handle_sync() {
    try {
        delay = boost::chrono::milliseconds(5000);  // Resetting delay to the initial value
        boost::property_tree::ptree pt;
        for (const auto& tuple : dw_ptr->getPaths()) {
            std::string path(tuple.first);
            path = path.substr(path_to_watch.size()+1);
            while (path.find('.') < path.size()) path.replace(path.find('.'), 1, ":");
            pt.add(path, tuple.second.hash);
        }
        std::stringstream map_stream;
        boost::property_tree::write_json(map_stream, pt, false);   // Saving the json in a stream
        std::string map_string = map_stream.str();
        Message write_msg;
        write_msg.encode_message(1, map_string);
        enqueue_msg(write_msg);
    } catch (const boost::property_tree::ptree_error &err) {
        throw;
    }
}

void Client::handle_status(Message msg) {
    try {
        msg.decode_message();
        auto status = static_cast<status_type>(msg.get_header());   /* Change header to status */
        std::string data = msg.get_data();
        switch (status) {
            case status_type::in_need : {
                std::string separator = "||";
                size_t pos = 0;
                std::string path;
                while ((pos = data.find(separator)) != std::string::npos) {
                    path = data.substr(0, pos);
                    data.erase(0, pos + separator.length());
                    std::string relative_path = path;
                    while (relative_path.find(':') < relative_path.size())
                        relative_path.replace(relative_path.find(':'), 1, ".");
                    std::ifstream inFile;
                    relative_path = std::string(path_to_watch + "/") + relative_path;
                    boost::property_tree::ptree pt;
                    read_file(relative_path, path, pt);
                    /* Writing message */
                    std::stringstream file_stream;
                    boost::property_tree::write_json(file_stream, pt, false);   // Saving the json in a stream
                    std::string file_string(file_stream.str());
                    Message write_msg;
                    write_msg.encode_message(2, file_string);
                    enqueue_msg(write_msg);
                }
                break;
            }
            case status_type::unauthorized : {
                std::cerr << "Unauthorized. ";
                close();
                break;
            }
            case status_type::service_unavailable : {
                auto wait = boost::chrono::milliseconds(delay);
                std::cout << "Server unavailable, retrying in " << wait.count() << " sec" << std::endl;
                boost::this_thread::sleep_for(delay);
                Message last_message;
                if (data == "login" || data == "Communication error") {
                    last_message.put_credentials(cred.username, cred.password);
                    enqueue_msg(last_message);
                } else {
                    handle_sync();
                }
                if (wait.count() >= 20) {
                    std::cerr << "Server unavailable. ";
                    close();
                } else {
                        delay *= 2;
                }
                break;
            }
            case status_type::wrong_action : {
                std::cerr << "Wrong action. ";
                close();
                break;
            }
            case status_type::authorized : {
                std::cout << "Authorized." << std::endl;
                do_start_directory_watcher();
                handle_sync();
                break;
            }
            default : {
                std::cout << "Operation completed." << std::endl;
            }
        }
    } catch (const boost::property_tree::ptree_error &err) {
        std::cerr << "Error while communicating with server, closing session. " << std::endl;
        close();
    } catch (const std::ios_base::failure &err) {
        std::cerr << "Error while synchronizing with server, closing session. " << std::endl;
        close();
    }
}

void Client::read_file(const std::string& relative_path, const std::string& path, boost::property_tree::ptree& pt) {
    std::ifstream inFile;
    bool reopen_done = false;
    do {
        try {
            inFile.open(relative_path, std::ios::in|std::ios::binary);
            std::vector<BYTE> buffer_vec;
            char ch;
            while (inFile.get(ch)) buffer_vec.emplace_back(ch);
            std::string encodedData = base64_encode(&buffer_vec[0], buffer_vec.size());
            pt.clear();
            pt.add("path", path);
            pt.add("hash", dw_ptr->getNode(relative_path).hash);
            pt.add("isFile", dw_ptr->getNode(relative_path).isFile);
            pt.add("content", encodedData);
        } catch (const std::ios_base::failure &err) {
            if (!reopen_done) {
                boost::this_thread::sleep_for(boost::chrono::milliseconds(2000));
                reopen_done = true;
            } else {
                throw;
            }
        } catch (const boost::property_tree::ptree_bad_data &err) {
            if (!reopen_done) {
                boost::this_thread::sleep_for(boost::chrono::milliseconds(2000));
                reopen_done = true;
            } else {
                throw;
            }
        }
    } while (reopen_done);
}

void Client::close() {
    *running_client = *running_watcher = false;
    boost::asio::post(io_context_, [this]() {
        if (socket_.is_open()) socket_.close();
        std::unique_lock ul(input_mutex);
        std::cerr << "Do you want to reconnect? (y/n): ";
        cv.wait(ul);
    });
}

Client::~Client() {
    if (input_reader.joinable()) input_reader.join();
    if (directory_watcher.joinable()) directory_watcher.join();
}
