#include "Server_Session.h"

Server_Session::Server_Session(tcp::socket &socket) : socket_(std::move(socket)) {}

void Server_Session::start() {
    do_read();
}

void Server_Session::do_read() {
    std::cout << "Reading message..." << std::endl;
    auto self(shared_from_this());
    boost::asio::async_read_until(socket_, read_buf, delimiter,
                                  [this, self](const boost::system::error_code ec, std::size_t length){
                                      if (!ec) {
                                          std::string str(boost::asio::buffers_begin(read_buf.data()),
                                                          boost::asio::buffers_begin(read_buf.data()) + read_buf.size());
                                          read_buf.consume(length);     // Cropping buffer in order to let the next do_read work properly
                                          str.resize(length);           // Cropping in order to erase residuals taken from the buffer
                                          Message msg;
                                          *msg.get_msg_ptr() = str;
                                          request_handler(msg);
                                          do_read();
                                      } else {
                                          if (!username.empty()) std::cout << "Client " << username << " disconnected, closing session..." << std::endl;
                                          else std::cerr << "Error during login phase, closing session..." << std::endl;
                                      }
                                  });
}

void Server_Session::do_write() {
    std::cout << "Writing message..." << std::endl;
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
                             boost::asio::dynamic_string_buffer(*write_queue_s.front().get_msg_ptr()),
                             [this, self](boost::system::error_code ec, std::size_t length) {
                                 if (!ec) {
                                     std::lock_guard lg(wq_mutex);      // Lock in order to guarantee thread safe pop operation
                                     write_queue_s.pop();
                                     if (!write_queue_s.empty()) do_write();
                                 } else {
                                     std::cerr << "Error inside do_write: " << ec.message() << std::endl;
                                 }
                             });
}

void Server_Session::enqueue_msg(const Message &msg) {
    std::lock_guard lg(wq_mutex);       // Lock in order to guarantee thread safe push operation
    bool write_in_progress = !write_queue_s.empty();
    write_queue_s.push(msg);
    if (!write_in_progress) do_write();     // Calling do_write only if it is not already running
}

std::string Server_Session::do_write_element(action_type header, const std::string& data) {
    try {
        boost::property_tree::ptree pt;
        std::stringstream data_stream;
        data_stream << data;
        boost::property_tree::read_json(data_stream, pt);      // Re-creating json from data stream
        auto path = pt.get<std::string>("path");
        auto hash = pt.get<std::size_t>("hash");
        bool isFile = pt.get<bool>("isFile");
        update_paths(path, hash);
        std::string directory = std::string("../../server/") + std::string(username);
        if (!boost::filesystem::is_directory(directory)) boost::filesystem::create_directory(directory);
        std::string relative_path = directory + std::string("/") + std::string(path);   // Creating actual filesystem path
        while (relative_path.find(':') < relative_path.size())     // Resetting the original path format of the file or directory
            relative_path.replace(relative_path.find(':'), 1, ".");
        if (header == action_type::create && !isFile) {     // Creating a directory with the specified name
            boost::filesystem::create_directory(relative_path);
        } else {        // Creating a file with the specified name
            auto content = pt.get<std::string>("content");
            std::vector<BYTE> decodedData = base64_decode(content);
            boost::filesystem::ofstream outFile(relative_path.data());
            if (!content.empty()) {
                outFile.write(reinterpret_cast<const char *>(decodedData.data()), decodedData.size());
                outFile.close();
            }
        }
        return path;
    } catch (const boost::property_tree::ptree_error &err) {
        throw;
    } catch (const std::ios_base::failure &err) {
        throw;
    }
}

void Server_Session::do_remove_element(const std::string& path) {
    std::lock_guard lg(paths_mutex);    // Lock in order to guarantee thread safe operations on paths map
    paths.erase(path);
    std::string directory = std::string("../../server/") + std::string(username);
    std::string relative_path = directory + std::string("/") + std::string(path);
    while (relative_path.find(':') < relative_path.size())     // Resetting the original path format of the file or directory
        relative_path.replace(relative_path.find(':'), 1, ".");
    boost::filesystem::remove_all(relative_path.data());
}

void Server_Session::update_paths(const std::string& path, size_t hash) {
    std::lock_guard lg(paths_mutex);    // Lock in order to guarantee thread safe operations on paths map
    paths[path] = hash;
}

Diff_paths Server_Session::compare_paths(ptree &client_pt) {
    std::vector<std::string> toAdd;
    for (auto &entry : client_pt) {     // Scanning received map in search for new elements
        auto it = paths.find(entry.first);
        if (it != paths.end()) {
            std::stringstream hash_stream(entry.second.data());
            size_t entry_hash;
            hash_stream >> entry_hash;
            if (it->second != entry_hash) toAdd.emplace_back(entry.first);
        } else {
            toAdd.emplace_back(entry.first);
        }
    }
    std::vector<std::string> toRem;
    for (auto &entry : paths) {     // Scanning local map in search for deprecated elements
        auto it = client_pt.find(entry.first);
        if (it == client_pt.not_found()) toRem.emplace_back(entry.first);
    }
    return {toAdd, toRem};
}

void Server_Session::request_handler(Message msg) {
    Message response_msg;
    std::string response_str;
    int status_type = 999;      // Setting status type to an unreachable (wrong) value
    try {
        msg.decode_message();
        auto header = static_cast<action_type>(msg.get_header());
        std::string data = msg.get_data();
        if (header != action_type::login && username.empty()) {
            status_type = 1;
            response_str = std::string("Login needed");
        } else {
            switch (header) {
                case (action_type::login) : {
                    auto credentials = msg.get_credentials();
                    auto count_avail = db.check_database(std::get<0>(credentials), std::get<1>(credentials));
                    if (std::get<1>(count_avail)) {         // If db is available
                        if (std::get<0>(count_avail)) {     // If there is a match
                            username = std::get<0>(credentials);
                            status_type = 0;
                            response_str = std::string("Access granted");
                        } else {
                            status_type = 1;
                            response_str = std::string("Access denied, try again");
                        }
                    } else {
                        status_type = 7;
                        response_str = std::string("login");
                    }
                    break;
                }
                case (action_type::synchronize) : {
                    boost::property_tree::ptree pt;
                    std::stringstream data_stream;
                    data_stream << data;
                    boost::property_tree::read_json(data_stream, pt);  // Re-creating json from data stream
                    auto found_avail = db.get_paths(paths, username);
                    if (std::get<1>(found_avail)) {     //  If the database is available
                        if (std::get<0>(found_avail)) {     // Comparing the maps and answering either with in_need o no_need
                            Diff_paths diffs = compare_paths(pt);
                            if (diffs.toAdd.empty()) {
                                status_type = 5;
                                response_str = "No need";
                            } else {
                                status_type = 6;
                                for (const auto &path : diffs.toAdd)
                                    response_str += path + "||";    // Adding missing paths to the response message
                            }
                            if (!diffs.toRem.empty()) {
                                for (const auto &path : diffs.toRem)
                                    do_remove_element(path);
                            }
                        } else {    // Answering being in_need with the whole map
                            status_type = 6;
                            for (const auto &path : pt) response_str += path.first + "||";
                        }
                    } else {
                        status_type = 7;
                        response_str = std::string("synchronize");
                    }
                    break;
                }
                case (action_type::create) : {
                    std::string path = do_write_element(header, data);
                    status_type = 2;
                    response_str = std::string(path) + std::string(" created");
                    break;
                }
                case (action_type::update) : {
                    std::string path = do_write_element(header, data);
                    status_type = 3;
                    response_str = std::string(path) + std::string(" updated");
                    break;
                }
                case (action_type::erase) : {
                    boost::property_tree::ptree pt;
                    std::stringstream data_stream;
                    data_stream << data;
                    boost::property_tree::read_json(data_stream, pt);  // Re-creating json from data stream
                    auto path = pt.get<std::string>("path");
                    do_remove_element(path);
                    status_type = 4;
                    response_str = std::string(path) + std::string(" erased");
                    break;
                }
                default : {
                    status_type = 8;
                    response_str = std::string("Wrong action type");
                }
            }
        }
        if (status_type <= 8) {     // In case of error no message is sent to the client
            response_msg.encode_message(status_type, response_str);
            enqueue_msg(response_msg);
        }
    } catch (const boost::property_tree::ptree_error &err) {
        response_str = std::string("Communication error");
        try {
            response_msg.encode_message(7, response_str);
            enqueue_msg(response_msg);
            std::cerr << "Server is not working properly." << std::endl;
        } catch (const boost::property_tree::ptree_error &err) {
            socket_.close();
        }
    } catch (const std::ios_base::failure &err) {
        response_str = std::string("Communication error");
        response_msg.encode_message(7, response_str);
        enqueue_msg(response_msg);
        std::cerr << "Server is not working properly." << std::endl;
    }
}

Server_Session::~Server_Session() {
    auto delay = boost::chrono::milliseconds(5000);
    while (delay.count() <= 20000) {
        if (!paths.empty()) {   // If the paths map has been loaded with the db copy, then update
            try {
                bool result;
                do {
                    result = db.update_db_paths(paths, username);
                    if (!result) {
                        std::cout << "Waiting for " << delay.count()/1000 << " sec..." << std::endl;
                        boost::this_thread::sleep_for(delay);   // Waiting for an increasing amount of time
                        delay *= 2;
                    }
                } while (!result && delay.count() <= 20000);    // Looping until either the db is correctly accessed or the delay is too high
                if (result) std::cout << "Database successfully updated" << std::endl;
                else std::cout << "Database not updated" << std::endl;
                delay = boost::chrono::milliseconds(30000);     // Setting the delay to a value that can break the external loop
            } catch (const boost::property_tree::ptree_error &err) {
                std::cout << "Waiting for " << delay.count()/1000 << " sec..." << std::endl;
                if (delay.count() <= 20000) {
                    boost::this_thread::sleep_for(delay);
                    delay *= 2;
                }
                if (delay.count() > 20000) std::cout << "Database not updated" << std::endl;
            }
        } else break;
    }
}
