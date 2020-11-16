#include "Server_Session.h"
#include "Base64/base64.h"

Server_Session::Server_Session(tcp::socket &socket) : socket_(std::move(socket)) {}

void Server_Session::update_paths(const std::string& path, size_t hash) {
    paths[path] = hash;
}

void Server_Session::do_remove(const std::string& path) {
    paths.erase(path);
    std::string directory = std::string("../../server/") + std::string(username);
    std::string relative_path = directory + std::string("/") + std::string(path);
    if (relative_path.find(':') < relative_path.size())
        relative_path.replace(relative_path.find(':'), 1, ".");
    boost::filesystem::remove_all(relative_path.data());
}

void Server_Session::start() {
    do_read_body();
}

void Server_Session::do_read_body() {
    std::cout << "Reading message body..." << std::endl;
    auto self(shared_from_this());
    boost::asio::async_read_until(socket_,buf, delimiter,
            [this, self](const boost::system::error_code ec, std::size_t length){
                if (!ec) {
                    std::string str(boost::asio::buffers_begin(buf.data()),
                            boost::asio::buffers_begin(buf.data()) + buf.size());
            buf.consume(length);
            Message msg;
            *msg.get_msg_ptr() = str;
            msg.get_msg_ptr()->resize(length);
            request_handler(msg);
            do_read_body();
        } else {
            std::cerr << "Error inside do_read_body: " << ec.message() << std::endl;
        }
    });
}

void Server_Session::do_write() {
    std::cout << "Writing message..." << std::endl;
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
            boost::asio::dynamic_string_buffer(*write_queue_s.front().get_msg_ptr()),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    write_queue_s.pop();
                    if (!write_queue_s.empty()) do_write();
                } else {
                    std::cerr << "Error inside do_write: " << ec.message() << std::endl;
                }
    });
}

Diff_vect Server_Session::compare_paths(ptree &client_pt) {
    std::vector<std::string> toAdd;     /* Scanning received map in search for new elements */
    for (auto &entry : client_pt) {
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
    std::vector<std::string> toRem; /* Scanning local map in search for deprecated elements */
    for (auto &entry : paths) {
        auto it = client_pt.find(entry.first);
        if (it == client_pt.not_found()) toRem.emplace_back(entry.first);
    }
    return {toAdd, toRem};
}

void Server_Session::enqueue_msg(const Message &msg) {
    bool write_in_progress = !write_queue_s.empty();
    write_queue_s.push(msg);
    if (!write_in_progress) do_write();
}

void Server_Session::request_handler(Message msg) {
    msg.decode_message();
    auto header = static_cast<action_type>(msg.get_header());
    std::string data = msg.get_data();
    int status_type;
    std::string response_str;
    Message response_msg;
    if (header != action_type::login && username.empty()) {
        status_type = 6;
        response_str = std::string("Login needed");
    } else {
        switch (header) {
            case (action_type::login) : {
                auto credentials = msg.get_credentials();
                auto count_avail = db.check_database(std::get<0>(credentials), std::get<1>(credentials));
                if (std::get<1>(count_avail)) {
                    if (std::get<0>(count_avail)) {
                        username = std::get<0>(credentials);
                        status_type = 0;
                        response_str = std::string("Access granted");
                    } else {
                        status_type = 6;
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
                boost::property_tree::json_parser::read_json(data_stream, pt);
                auto found_avail = db.get_paths(paths, username);
                if (std::get<1>(found_avail)) {
                    // vede se il database risponde
                    if (std::get<0>(found_avail)) {
                        // deve confrontare le mappe e rispondere con in_need o no_need
                        Diff_vect diffs = compare_paths(pt);
                        if (diffs.toAdd.empty()) {
                            status_type = 4;
                            response_str = "No need";
                        } else {
                            status_type = 5;
                            for (const auto &path : diffs.toAdd) response_str += path + "||";
                        }
                        if (!diffs.toRem.empty()) {
                            for (const auto &path : diffs.toRem) do_remove(path);
                        }
                    } else {
                        // deve rispondere in_need con tutta la mappa ricevuta come dati
                        status_type = 5;
                        for (const auto &path : pt) response_str += path.first + "||";
                    }
                } else {
                    status_type = 7;
                    response_str = std::string("synchronize");
                }
                break;
            }
            case (action_type::create) : {
                boost::property_tree::ptree pt;
                std::stringstream data_stream;
                data_stream << data;
                boost::property_tree::json_parser::read_json(data_stream, pt);
                auto path = pt.get<std::string>("path");
                auto hash = pt.get<std::size_t>("hash");
                bool isFile = pt.get<bool>("isFile");
                update_paths(path, hash);
                std::string directory = std::string("../../server/") + std::string(username);
                if (!boost::filesystem::is_directory(directory)) boost::filesystem::create_directory(directory);
                std::string relative_path = directory + std::string("/") + std::string(path);
                if (!isFile) {
                    // create a directory with the specified name
                    boost::filesystem::create_directory(relative_path);
                } else {
                    // create a file with the specified name
                    auto content = pt.get<std::string>("content");
                    std::vector<BYTE> decodedData = base64_decode(content);
                    if (relative_path.find(':') < relative_path.size())
                        relative_path.replace(relative_path.find(':'), 1, ".");
                    boost::filesystem::ofstream outFile(relative_path.data());
                    if (!content.empty()) {
                        outFile.write(reinterpret_cast<const char *>(decodedData.data()), decodedData.size());
                        outFile.close();
                    }
                }
                status_type = 1;
                response_str = std::string(path) + std::string(" created");
                break;
            }
            case (action_type::update) : {
                boost::property_tree::ptree pt;
                std::stringstream data_stream;
                data_stream << data;
                boost::property_tree::json_parser::read_json(data_stream, pt);
                auto path = pt.get<std::string>("path");
                auto hash = pt.get<std::size_t>("hash");
                bool isFile = pt.get<bool>("isFile");
                update_paths(path, hash);
                if (isFile) {
                    auto content = pt.get<std::string>("content");
                    std::vector<BYTE> decodedData = base64_decode(content);
                    std::string directory = std::string("../../server/") + std::string(username);
                    std::string relative_path = directory + std::string("/") + std::string(path);
                    if (relative_path.find(':') < relative_path.size())
                        relative_path.replace(relative_path.find(':'), 1, ".");
                    boost::filesystem::remove_all(relative_path.data());
                    boost::filesystem::ofstream outFile(relative_path.data());
                    if (!content.empty()) {
                        outFile.write(reinterpret_cast<const char *>(decodedData.data()), decodedData.size());
                        outFile.close();
                    }
                }
                status_type = 2;
                response_str = std::string(path) + std::string(" updated");
                break;
            }
            case (action_type::erase) : {
                boost::property_tree::ptree pt;
                std::stringstream data_stream;
                data_stream << data;
                boost::property_tree::json_parser::read_json(data_stream, pt);
                auto path = pt.get<std::string>("path");
                do_remove(path);
                status_type = 3;
                response_str = std::string(path) + std::string(" erased");
                break;
            }
            default : {
                status_type = 8;
                response_str = std::string("Wrong action type");
            }
        }
    }
    response_msg.encode_message(status_type, response_str);
    enqueue_msg(response_msg);
}

Server_Session::~Server_Session() {
    if (!paths.empty()) {
        bool result = db.update_db_paths(paths, username);
        if (!result) {
            Message response_msg;
            std::string response_str("Service unavailable");
            response_msg.encode_message(7, response_str);
            enqueue_msg(response_msg);
        }
    }
}
