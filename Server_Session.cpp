#include "Server_Session.h"

Server_Session::Server_Session(tcp::socket &socket) : socket_(std::move(socket)), successful_first_loading(false) {}

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

std::string Server_Session::do_write_element(action_type header, const boost::property_tree::ptree& data_pt) {
    try {
        std::lock_guard lg(fs_mutex);
        auto path = data_pt.get<std::string>("path", "none");
        if (path == "none")
            log_and_report("Communication error", "Error while decoding the path to write. ");
        auto hash = data_pt.get<std::string>("hash", "none");
        if (hash == "none")
            log_and_report("Communication error", "Error while decoding the hash of the element that has to be written. ");
        auto isFile = data_pt.get<int>("isFile", 999);
        if (isFile == 999)
            log_and_report("Communication error", "Error while decoding the type of the element that has to be written. ");
        std::string directory = std::string("../../server/") + std::string(username);
        if (!boost::filesystem::is_directory(directory)) boost::filesystem::create_directory(directory);
        std::string relative_path = directory + std::string("/") + std::string(path);   // Creating actual filesystem path
        while (relative_path.find(':') < relative_path.size())     // Resetting the original path format of the file or directory
            relative_path.replace(relative_path.find(':'), 1, ".");
        if (header == action_type::create && !isFile) {     // Creating a directory with the specified name
            boost::filesystem::create_directory(relative_path);
            update_paths(path, hash);
        } else {        // Creating a file with the specified name
            auto content = data_pt.get<std::string>("content", "none");
            if (content == "none")
                log_and_report("Communication error", "Error while decoding the content fo the file that has to be written. ");
            std::vector<BYTE> decodedData = base64_decode(content);
            boost::filesystem::ofstream outFile(relative_path.data());
            if (outFile.write(reinterpret_cast<const char *>(decodedData.data()), decodedData.size()).good()) {
                update_paths(path, hash);
            } else {
                socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                socket_.close();
            }
        }
        return path;
    } catch (const std::ios_base::failure &err) {
        return "";
    }
}

void Server_Session::do_remove_element(const std::string& path) {
    std::scoped_lock lg(paths_mutex, fs_mutex);    // Lock in order to guarantee thread safe operations on paths map and filesystem
    std::string directory = std::string("../../server/") + std::string(username);
    std::string relative_path = directory + std::string("/") + std::string(path);
    while (relative_path.find(':') < relative_path.size())     // Resetting the original path format of the file or directory
        relative_path.replace(relative_path.find(':'), 1, ".");
    /*if (!boost::filesystem::remove_all(relative_path.data()))
        log_and_report("Communication error", "Error while deleting an element. ");*/
    boost::filesystem::remove_all(relative_path.data());
    paths.erase(path);
    update_db();
}

void Server_Session::update_paths(const std::string& path, const std::string& hash) {
    std::lock_guard lg(paths_mutex);    // Lock in order to guarantee thread safe operations on paths map
    paths[path] = hash;
    update_db();
}

void Server_Session::update_db() {
    auto delay = boost::chrono::seconds (5);
    if (successful_first_loading) {   // If the paths map has been loaded with the db copy, then update
        bool result;
        do {
            result = db.update_db_paths(paths, username);
            if (!result) {
                std::cout << "Waiting for " << delay.count() << " sec..." << std::endl;
                boost::this_thread::sleep_for(delay);   // Waiting for an increasing amount of time
                delay *= 2;
            }
        } while (!result && delay.count() <= 20);    // Looping until either the db is correctly accessed or the delay is too high
        if (result) std::cout << "Database successfully updated" << std::endl;
        else std::cout << "Database not updated" << std::endl;
    }
}

Diff_paths Server_Session::compare_paths(ptree &client_pt) {
    std::vector<std::string> toAdd;
    std::vector<std::string> toRem;
    for (auto &entry : client_pt) {     // Scanning received map in search for new elements
        if (entry.first == "empty") {
            toAdd.clear();
            break;  // If the json contains the message "empty: directory" then there is nothing to add in the server
        }
        auto it = paths.find(entry.first);
        if (it != paths.end()) {
            std::string entry_hash(entry.second.data());
            auto pt_hash = it->second;
            if (pt_hash != entry_hash) toAdd.emplace_back(entry.first);
        } else {
            toAdd.emplace_back(entry.first);
        }
    }
    for (auto &entry : paths) {     // Scanning local map in search for deprecated elements
        auto it = client_pt.find(entry.first);
        if (it == client_pt.not_found()) toRem.emplace_back(entry.first);
    }
    return {toAdd, toRem};
}

void Server_Session::request_handler(Message msg) {
    std::string response_str;
    int status_type = 999;      // Setting status type to an unreachable (wrong) value
    if (!msg.decode_message())
        log_and_report("Communication error", "Error while decoding client message. ");
    int header_int = msg.get_header();
    auto data_pt = msg.get_pt_data();
    if ((data_pt.empty() && header_int != 0) || header_int == 999)
        log_and_report("Communication error", "Error while getting fields from message. ");
    auto header = static_cast<action_type>(header_int);
    if (header != action_type::login && username.empty()) {
        status_type = 1;
        response_str = std::string("Login needed");
    } else {
        switch (header) {
            case (action_type::login) : {
                auto credentials = msg.get_credentials();
                if (std::get<0>(credentials) == "error" || std::get<1>(credentials) == "error") log_and_report("Communication error", "Error while decoding credentials. ");
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
                auto found_avail = db.get_paths(paths, username);
                if (std::get<1>(found_avail)) {     //  If the database is available
                    successful_first_loading = true;
                    if (std::get<0>(found_avail)) {     // Comparing the maps and answering either with in_need o no_need
                        Diff_paths diffs = compare_paths(data_pt);
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
                        for (const auto &path : data_pt) response_str += path.first + "||";
                    }
                } else {
                    status_type = 7;
                    response_str = std::string("synchronize");
                }
                break;
            }
            case (action_type::create) : {
                std::string path = do_write_element(header, data_pt);
                if (path.empty()) log_and_report("Communication error", "Error while creating the element");
                status_type = 2;
                response_str = std::string(path) + std::string(" created");
                break;
            }
            case (action_type::update) : {
                std::string path = do_write_element(header, data_pt);
                if (path.empty()) log_and_report("Communication error", "Error while updating the element");
                status_type = 3;
                response_str = std::string(path) + std::string(" updated");
                break;
            }
            case (action_type::erase) : {
                auto path = data_pt.get<std::string>("path", "none");
                if (path == "none") log_and_report("Communication error", "Error while decoding the path of the file that has to be erased. ");
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
        Message response_msg;
        if (!response_msg.encode_message(status_type, response_str)) log_and_report("Communication error", "Server is not working properly.");
        enqueue_msg(response_msg);
    }
}

void Server_Session::log_and_report(const std::string& response, const std::string& log) {
    Message response_msg;
    std::cerr << log << std::endl;
    if (!response_msg.encode_message(7, response)) {
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        socket_.close();
    }
    enqueue_msg(response_msg);
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    socket_.close();
}

Server_Session::~Server_Session() {
    update_db();
}
