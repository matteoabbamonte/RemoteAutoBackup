#include "Client.h"

#define delimiter "\n}\n"
#define defaultValue "d4e5rf6t7nyn7hmj8m9j9mm9j8n7h6gb5fc4x3wwx5cgb78nhm9j"

Client::Client(boost::asio::io_context& io_context,
               tcp::resolver::results_type endpoints,
               std::string path_to_watch,
               std::shared_ptr<DirectoryWatcher> &dw,
               std::shared_ptr<bool> &stop,
               std::shared_ptr<bool> &running_client,
               std::shared_ptr<bool> &running_watcher)
        : io_context_(io_context),
          socket_(io_context),
          endpoints(std::move(endpoints)),
          path_to_watch(std::move(path_to_watch)),
          dw_ptr(dw),
          stop(stop),
          running_client(running_client),
          running_watcher(running_watcher),
          delay(5) {
    do_connect();
}

void Client::do_connect() {
    std::cout << "Trying to connect..." << std::endl;
    boost::asio::async_connect(socket_, endpoints, [this](boost::system::error_code ec, const tcp::endpoint&) { // Asynchronous connection request
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
            read_buf.consume(length);     // Cropping buffer in order to let the next do_read work properly
            str.resize(length);           // Cropping in order to erase residuals taken from the buffer
            Message msg;
            *msg.get_msg_ptr() = str;
            handle_status(msg);
            do_read();
        } else {
            *running_watcher = false;   // Signaling to the directory watcher the end of the client session
            if (*running_client) handle_reading_failures();   // If the socket has been closed by the server, then call the EOF handler
        }
    });
}

void Client::do_write() {
    std::cout << "Writing message..." << std::endl;
    auto msg = write_queue_c.front();
    boost::asio::async_write(socket_, boost::asio::dynamic_string_buffer(*write_queue_c.front().get_msg_ptr()),
                             [this, msg](boost::system::error_code ec, std::size_t length) {
                                 if (!ec) {
                                     auto response_timer = std::make_unique<boost::asio::system_timer>(io_context_, boost::asio::chrono::minutes(10));   // Setting a 10 minutes timer
                                     response_timer->async_wait([this](const boost::system::error_code &error){                                      // after which, if the client
                                         if (!error) log_and_close("Timeout expired, closing session.");                                            // didn't receive the corresponding
                                     });                                                                                                                     // ack from the server then logs the
                                     std::string key;                                                                                                        // error and closes the current session
                                     int header_int = const_cast<Message&>(msg).get_header();
                                     if (header_int == 999) log_and_close("Error while getting the header of the message for setting timeout. ");
                                     auto header = static_cast<action_type>(header_int);
                                     switch (header) {
                                         case action_type::login : {
                                             key = "login";
                                             break;
                                         }
                                         case action_type::synchronize : {
                                             key = "synch";
                                             break;
                                         }
                                         default: {
                                             auto data = const_cast<Message&>(msg).get_pt_data();
                                             if (data.empty()) {
                                                 try {
                                                     response_timer->cancel();   // if data is empty due to an error then the timer is canceled in order to avoid the shutdown
                                                 } catch (const boost::system::system_error &err) {
                                                     std::cerr << "An error occurred during the timer cancelling process, shutting down in less than 10 minutes." << std::endl;
                                                 };
                                                 if (*running_client) {
                                                     *running_watcher = false;
                                                     log_and_close("Error while getting data for setting timeout. ");
                                                 }
                                                 break;
                                             }
                                             key = data.get<std::string>("path", defaultValue);
                                             if (key == defaultValue) {
                                                 try {
                                                     response_timer->cancel();   // if key is "none" due to an error then the timer is canceled in order to avoid the shutdown
                                                 } catch (const boost::system::system_error &err) {
                                                     std::cerr << "An error occurred during the timer cancelling process, shutting down in less than 10 minutes." << std::endl;
                                                 };
                                                 if (*running_client) {
                                                     *running_watcher = false;
                                                     log_and_close("Error while getting path key for setting timeout. ");
                                                 }
                                                 break;
                                             }
                                             if (header == action_type::create) key += std::string(" created");
                                             else if (header == action_type::update) key += std::string(" updated");
                                             else key += std::string(" erased");
                                             break;
                                         }
                                     }
                                     ack_tracker[key] = std::move(response_timer);   // Adding the created timer to the ack tracker map
                                     std::lock_guard lg(wq_mutex);   // Lock in order to guarantee thread safe pop operation
                                     write_queue_c.pop();
                                     if (!write_queue_c.empty()) do_write();
                                 } else {
                                     if (*running_client) {
                                         *running_watcher = false;   // Signaling to the directory watcher the end of the client session
                                         log_and_close("Error while writing. ");
                                     }
                                 }
                             });
}

void Client::enqueue_msg(const Message &msg) {
    std::lock_guard lg(wq_mutex);   // Lock in order to guarantee thread safe push operation
    bool write_in_progress = !write_queue_c.empty();
    write_queue_c.push(msg);
    if (!write_in_progress) do_write();    // Calling do_write only if it is not already running
}

void Client::get_credentials() {
    std::unique_lock ul(input_mutex);   // Unique lock in order to use the cv wait
    do_start_input_reader();
    cv.wait(ul, [this](){return !cred.username.empty() && !cred.password.empty();});    // Waiting for the input reader thread to receive the credentials
    Message login_message;
    if (!login_message.put_credentials(cred.username, cred.password))  // Saving the credentials in the message that has to be sent
        log_and_close("Error while completing login procedure. ");
    enqueue_msg(login_message);
}

void Client::set_username(std::string &user) {
    cred.username = user;
}

void Client::set_password(std::string &pwd) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;  // Initializing and creating a new digest based on the password parameter
    if (!SHA256_Init(&sha256) || !SHA256_Update(&sha256, pwd.data(), pwd.length()) || !SHA256_Final(digest, &sha256))
        log_and_close("Error while hashing password");
    std::stringstream ss;
    for (auto ch : digest)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int) ch;    // Creating a stream starting from the digest
    cred.password = ss.str();
}

void Client::do_start_input_reader() {
    input_reader = boost::thread([this](){
        std::string input;
        bool user_done = false;    // True if the username has been inserted
        bool cred_done = false;    // True if the credentials have been inserted
        std::cout << "Insert username: ";
        while (std::cin >> input) {
            if (!std::cin) log_and_close("Error during input process");    // If there is any error during the input process, then close.
            if (cred_done) {
                if (input == "exit") {
                    if (*running_client) close();   // If the input is equal to 'exit' and the client session is still up, then close
                    else std::cerr << "Do you want to reconnect? (y/n): ";
                } else if (input == "y") {     // If the input is equal to 'y', then the thread sets the value of stop to false and notifies the input to the client thread
                    std::lock_guard lg(input_mutex);
                    *stop = false;
                    cv.notify_all();    // Unlocks close() wait
                    break;
                } else if (input == "n") {     // If the input is equal to 'n', then the thread sets the value of stop to true and notifies the input to the client thread
                    std::lock_guard lg(input_mutex);
                    *stop = true;
                    cv.notify_all();    // Unlocks close() wait
                    break;
                } else if (!*running_client){   // If the input is not an expected input and the client is not running anymore, then repeat the question
                    std::cerr << "Do you want to reconnect? (y/n): ";
                }
            } else {
                std::lock_guard lg(input_mutex);
                if (!user_done) {   // If the username has not been inserted, then its set function is called on the input
                    set_username(input);
                    user_done = true;
                    std::cout << "Insert password: ";
                } else {            // Else the password set function is called on the input
                    set_password(input);
                    cred_done = true;
                    cv.notify_all();    // Waking up the client thread in the get_credentials function
                }
            }
        }
    });
}

void Client::do_start_directory_watcher() {
    if (!*running_watcher) *running_watcher = true;    // If the reconnection attempt went smoothly, then restart the directory watcher
    directory_watcher = boost::thread([this](){
        dw_ptr->start([this](std::string path, FileStatus status, bool isFile) {
            if (boost::filesystem::is_regular_file(boost::filesystem::path(path))   // Process only regular files, all other file types are ignored
                || boost::filesystem::is_directory(boost::filesystem::path(path)) || status == FileStatus::erased) {
                boost::property_tree::ptree pt;
                int action_type = 999;     // Setting action type to an unreachable (wrong) value
                if (path_to_watch.size() + 1 >= path.size()) log_and_close("Error while truncating the path of the element. ");
                std::string path_to_send = path.substr(path_to_watch.size() + 1);   // Preparing only the name of the file or directory
                while (path_to_send.find('.') < path_to_send.size())    // Making the path compatible with json polices
                    path_to_send.replace(path_to_send.find('.'), 1, ":");
                switch (status) {
                    case FileStatus::created : {
                        if (isFile) std::cout << "File created: " << path_to_send << '\n';
                        else std::cout << "Directory created: " << path_to_send << '\n';
                        if (!read_file(path, path_to_send, pt)) {
                            std::cerr << "Error while opening the file: " << path_to_send << " It won't be sent." << std::endl;
                            paths_to_ignore.emplace_back(path_to_send);    // Adding the path of the file to the black list for removal
                            break;
                        }
                        action_type = 2;
                        break;
                    }
                    case FileStatus::modified : {
                        if (isFile) {
                            std::cout << "File modified: " << path << '\n';
                            action_type = 3;
                        } else {
                            std::cout << "Directory modified: " << path << '\n';
                            action_type = 2;
                        }
                        if (!read_file(path, path_to_send, pt)) {
                            std::cerr << "Error while opening the file: " << path_to_send << " It won't be sent." << std::endl;
                            paths_to_ignore.emplace_back(path_to_send);    // Adding the path of the file to the black list for removal
                            break;
                        }
                        break;
                    }
                    case FileStatus::erased : {
                        try {
                            if (std::find(paths_to_ignore.begin(), paths_to_ignore.end(), path_to_send) == paths_to_ignore.end()) {    // If the path is not blacklisted, then send the delete command
                                pt.add("path", path_to_send);
                                action_type = 4;
                                if (isFile) std::cout << "File erased: " << path_to_send << '\n';
                                else std::cout << "Directory erased: " << path_to_send << '\n';
                            }
                        } catch (const boost::property_tree::ptree_error &err) {
                            log_and_close("Error while executing the action on the file " + path_to_send + ", closing session. ");
                        }
                        break;
                    }
                    default :
                        std::cout << "Error! Unknown file status.\n";
                }
                // Writing message
                if (action_type <= 4) {    // If no errors occurred
                    Message write_msg;
                    if (!write_msg.encode_message(action_type, pt)) {
                        paths_to_ignore.emplace_back(path_to_send);    // Adding the path of the file to the black list for removal
                        std::cerr << "Error while executing the action on the file " << path_to_send << ", it won't be sent. " << std::endl;
                        std::cerr << "If you want to resynchronize write \'exit\'." << std::endl;
                    }
                    enqueue_msg(write_msg);
                }
            }
        });
    });
}

void Client::handle_connection_failures() {
    boost::asio::async_connect(socket_, endpoints, [this](boost::system::error_code ec, const tcp::endpoint&) {    // Retrying the connection request to the socket
        if (!ec) {
            delay = boost::chrono::seconds(5);  // Resetting delay to the initial value
            get_credentials();
            do_read();
        } else {
            auto wait = boost::chrono::seconds(delay);
            std::cout << "Server unavailable, retrying in " << wait.count() << " sec" << std::endl;
            boost::this_thread::sleep_for(delay);
            if (wait.count() >= 20) {
                std::cerr << "Server unavailable. ";
                std::cerr << "Do you want to reconnect? (y/n): ";
                std::string input;
                while (std::cin >> input) {
                    if (!std::cin) close();        // If there is any error during the input process, then close.
                    if (input == "n") {            // If the input is 'n', then stops the while in the main
                        *stop = true;
                        break;
                    } else if (input == "y") {     // If the input is 'y', then take another round in the while in the main
                        break;
                    } else {
                        std::cerr << "Do you want to reconnect? (y/n): ";
                    }
                }
            } else {
                delay *= 2;    // If the wait is not over the limit and there is an error, then double the delay and recall the function
                handle_connection_failures();
            }
        }
    });
}

void Client::handle_reading_failures() {
    boost::asio::async_connect(socket_, endpoints, [this](boost::system::error_code ec, const tcp::endpoint&) {    // Retrying the connection request to the socket
        if (!ec) {
            delay = boost::chrono::seconds(5);  // Resetting delay to the initial value
            timer.stop();   // Stopping the timer in order to measure the time between one failure and the following one
            Message login_message;
            if (!login_message.put_credentials(cred.username, cred.password))  // Re-creating the login message with the saved credentials in order to automatize the reconnection attempt
                log_and_close("Error while reconnecting. ");
            enqueue_msg(login_message);
            do_start_directory_watcher();   // Restarting the directory watcher if the reconnection goes well
            do_read();   // Restarting the reading from socket procedure if the reconnection goes well
            handle_reconnection_timer();
        } else {
            auto wait = boost::chrono::seconds(delay);
            std::cout << "Server unavailable, retrying in " << wait.count() << " sec" << std::endl;
            boost::this_thread::sleep_for(delay);
            if (wait.count() >= 20) {
                log_and_close("Server unavailable. ");
            } else {
                delay *= 2;    // If the wait is not over the limit and there is an error, then double the delay and recall the function
                handle_reading_failures();
            }
        }
    });
}

void Client::handle_reconnection_timer() {
    if (timer.is_stopped()) timer.resume();
    else timer.start();
    reconnection_counter++;
    auto elapsed_seconds = timer.elapsed().system/(1e9);    // Taking the number of elapsed nanoseconds scaled of 1 Billion
    if (reconnection_counter > 1000 && elapsed_seconds <= 1) {     // If the frequency of reconnection attempts is too high then close
        log_and_close("Too many reconnection attempts.");
    } else if (elapsed_seconds > 1) {   // Else if the frequency is not too high then reset the counter and the timer
        reconnection_counter = 0;
        timer.elapsed().clear();
    }
}

int Client::handle_sync() {
    int res;
    try {
        delay = boost::chrono::seconds(5);  // Resetting delay to the initial value
        boost::property_tree::ptree pt;
        for (const auto& tuple : dw_ptr->getPaths()) {  // Looping over the path map
            std::string path(tuple.first);
            if (path_to_watch.size()+1 >= path.size()) log_and_close("Error while truncating the path of the element. ");
            path = path.substr(path_to_watch.size()+1);    // Taking only the file or directory name
            while (path.find('.') < path.size())
                path.replace(path.find('.'), 1, ":");    // Making the path compatible with json polices
            pt.add(path, tuple.second.hash);    // Adding the node to the json
        }
        Message write_msg;
        if (pt.empty()) pt.add("empty", "directory");   // Making the server aware that the path to watch is empty
        if (!write_msg.encode_message(1, pt)) {
            res = 0;
        } else {
            enqueue_msg(write_msg);
            res = 1;
        }
    } catch (const boost::property_tree::ptree_error &err) {
        res = 0;
    }
    return res;
}

void Client::handle_status(Message msg) {
    if (!msg.decode_message()) log_and_close("Error while decoding server message, closing session. ");
    int status = msg.get_header();
    auto data = msg.get_str_data();
    if (data.empty() || status == 999)
        log_and_close("Error while communicating with server, closing session. ");
    switch (status) {
        case status_type::in_need : {
            try {
                ack_tracker["synch"]->cancel();    // Canceling the timer related to the "synch" ack
            } catch (const boost::system::system_error &err) {
                std::cerr << "An error occurred during the timer cancelling process, shutting down in less than 10 minutes." << std::endl;
            };
            ack_tracker.erase("synch");    // Erasing the corresponding element from the ack map
            std::string separator = "||";
            size_t pos;
            std::string path_to_send;
            while ((pos = data.find(separator)) != std::string::npos) {
                path_to_send = data.substr(0, pos);    // Taking the string section until the separator
                data.erase(0, pos + separator.length());    // Deleting the taken section of the string
                std::string path = path_to_send;
                while (path.find(':') < path.size())    // Resetting the original path format of the file or directory
                    path.replace(path.find(':'), 1, ".");
                path = std::string(path_to_watch + "/").append(path);   // Restoring the 'relative' path of the file or directory
                boost::property_tree::ptree pt;
                if (!read_file(path, path_to_send, pt))
                    log_and_close("Error while opening the file: " + path_to_send + " It won't be sent.");
                // Writing message
                Message write_msg;
                if (!write_msg.encode_message(2, pt)) {
                    log_and_close("Error while encoding message, closing session. ");
                    break;
                }
                enqueue_msg(write_msg);
            }
            break;
        }
        case status_type::no_need : {
            try {
                ack_tracker["synch"]->cancel();    // Canceling the timer related to the "synch" ack
            } catch (const boost::system::system_error &err) {
                std::cerr << "An error occurred during the timer cancelling process, shutting down in less than 10 minutes." << std::endl;
            };
            ack_tracker.erase("synch");    // Erasing the corresponding element from the ack map
            break;
        }
        case status_type::unauthorized : {
            std::cerr << "Unauthorized. ";
            try {
                ack_tracker["login"]->cancel();    // Canceling the timer related to the "login" ack
            } catch (const boost::system::system_error &err) {
                std::cerr << "An error occurred during the timer cancelling process, shutting down in less than 10 minutes." << std::endl;
            };
            ack_tracker.erase("login");    // Erasing the corresponding element from the ack map
            close();    // If the login process failed, then close the current session
            break;
        }
        case status_type::service_unavailable : {
            auto wait = boost::chrono::seconds(delay);
            std::cout << "Server unavailable, retrying in " << wait.count() << " sec" << std::endl;
            boost::this_thread::sleep_for(delay);
            if (data == "login" || data == "Communication error") {    // If the server failed during login or any other process except from synchronization
                Message login_message;
                if (!login_message.put_credentials(cred.username, cred.password))  // then send the credentials again to the server and retry the login
                    log_and_close("Error while communicating with server, closing session. ");
                enqueue_msg(login_message);
            } else {
                if (!handle_sync()) log_and_close("Error while communicating with server, closing session. ");     // Else retry the synchronization procedure
            }
            if (wait.count() >= 20) {
                log_and_close("Server unavailable. ");
            } else {
                delay *= 2;    // If the wait is not over the limit and there is an error, then double the delay and restart the loop
            }
            break;
        }
        case status_type::wrong_action : {
            log_and_close("Wrong action. ");    // If a wrong action is recognized by the server and sent back, then close the current session
            break;
        }
        case status_type::authorized : {
            std::cout << "Authorized." << std::endl;
            try {
                ack_tracker["login"]->cancel();    // Canceling the timer related to the "login" ack
            } catch (const boost::system::system_error &err) {
                std::cerr << "An error occurred during the timer cancelling process, shutting down in less than 10 minutes." << std::endl;
            };
            ack_tracker.erase("login");    // Erasing the corresponding element from the ack map
            do_start_directory_watcher();   // Starting the directory watcher
            if (!handle_sync()) log_and_close("Error while communicating with server, closing session. ");  // Starting the synchronization procedure
            break;
        }
        default : {
            std::cout << "Operation completed." << std::endl;
            try {
                //std::cout << data.substr(0, data.rfind(' ')) << std::endl;
                ack_tracker[data]->cancel();    // Canceling the timer related to the "sent data" ack
            } catch (const boost::system::system_error &err) {
                std::cerr << "An error occurred during the timer cancelling process, shutting down in less than 10 minutes." << std::endl;
            };
            ack_tracker.erase(data);    // Erasing the corresponding element from the ack map
        }
    }
}

int Client::read_file(const std::string& path, const std::string& path_to_send, boost::property_tree::ptree& pt) {
    std::ifstream inFile;
    int res;
    try {
        std::lock_guard lg(fs_mutex);   // Lock in order to guarantee thread safe read operation
        inFile.open(path, std::ios::in|std::ios::binary);
        if (!inFile) res = 0;   // Opening the file in binary mode
        else {
            std::vector<BYTE> buffer_vec;   // Creating an unsigned char vector
            char ch;
            while (inFile.get(ch)) buffer_vec.emplace_back(ch);    // Adding every char read from the file to the vector
            std::string encodedData = base64_encode(&buffer_vec[0], buffer_vec.size());
            pt.add("path", path_to_send);
            pt.add("hash", dw_ptr->getNode(path).hash);        // Retrieving the hash from the Node_Info struct of the directory watcher
            pt.add("isFile", dw_ptr->getNode(path).isFile ? 1 : 0);    // Retrieving the data type from the Node_Info struct of the directory watcher
            pt.add("content", encodedData);
            res = 1;
        }
    } catch (const boost::property_tree::ptree_bad_data &err) {
        res = 0;
    }
    return res;
}

void Client::log_and_close(const std::string& message) {
    std::cerr << message << std::endl;
    close();
}

void Client::close() {
    *running_client = *running_watcher = false;           // Closing watcher thread and setting the client session to not running
    for (auto & it : ack_tracker) it.second->cancel();    // Canceling every timer in the ack_tracker map
    boost::asio::post(io_context_, [this]() {   // Requesting the io_context to invoke the given handler and returning immediately
        if (socket_.is_open()) {
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            socket_.close();    // Closing the socket
        }
        std::unique_lock ul(input_mutex);             // Unique lock in order to use the cv wait
        std::cerr << "Do you want to reconnect? (y/n): ";
        cv.wait(ul);    // Waiting for the user decision about the reconnection attempt
    });
}

Client::~Client() {
    if (input_reader.joinable()) input_reader.join();             // Joining the input reader thread before shutting down
    if (directory_watcher.joinable()) directory_watcher.join();   // Joining the directory watcher thread before shutting down
}