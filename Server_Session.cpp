#include "Server_Session.h"
#include "Base64/base64.h"

Server_Session::Server_Session(tcp::socket &socket) : socket_(std::move(socket)), server_availability(true) {}

void Server_Session::update_paths(const std::string& path, size_t hash) {
    paths[path] = hash;
}

void Server_Session::remove_path(const std::string& path) {
    paths.erase(path);
}

void Server_Session::start() {
    do_read_body();
}

void Server_Session::do_read_body() {
    std::cout << "Reading message body..." << std::endl;
    auto self(shared_from_this());
    boost::asio::async_read_until(socket_,
                                  buf,
                                  delimiter,
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
            std::cout << "Error inside do_read_body: ";
            std::cout << ec.message() << std::endl;
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
                    if (!write_queue_s.empty()) {
                        do_write();
                    }
                } else {
                    std::cout << "Error inside do_write: ";
                    std::cout << ec.message() << std::endl;
                }
    });
}

bool Server_Session::check_database(const std::string& temp_username, const std::string& password) {
    std::cout << "Checking Database..." << std::endl;
    sqlite3* conn;
    int count = 0;
    if (sqlite3_open("../Clients.sqlite", &conn) == SQLITE_OK) {
        std::string sqlStatement = std::string("SELECT COUNT(*) FROM Client WHERE username = '") + temp_username + std::string("' AND password = '") + password + std::string("';");
        sqlite3_stmt *statement;
        int res = sqlite3_prepare_v2(conn, sqlStatement.c_str(), -1, &statement, nullptr);
        if (res == SQLITE_OK) {
            while( sqlite3_step(statement) == SQLITE_ROW ) {
                count = sqlite3_column_int(statement, 0);
            }
        } else {
            std::cout << "Database Error: " << res << ", " << sqlite3_errmsg(conn) << std::endl;
        }
        sqlite3_finalize(statement);
        sqlite3_close(conn);
    }
    return count;
}

void Server_Session::update_db_paths() {
    std::cout << "Updating Database..." << std::endl;
    boost::property_tree::ptree pt;
    for (auto & path : paths) {
        pt.add(path.first, path.second);
    }
    std::stringstream map_to_stream;
    boost::property_tree::write_json(map_to_stream, pt);
    sqlite3* conn;
    if (sqlite3_open("../Clients.sqlite", &conn) == SQLITE_OK) {
        std::string sqlStatement = std::string("UPDATE client SET paths = '") + map_to_stream.str() + std::string("' WHERE username = '") + username + std::string("';");
        sqlite3_stmt *statement;
        int res = sqlite3_prepare_v2(conn, sqlStatement.c_str(), -1, &statement, nullptr);
        if (res == SQLITE_OK) {
            sqlite3_step(statement);
        } else {
            std::cout << "Database Error: " << res << ", " << sqlite3_errmsg(conn) << std::endl;
        }
        sqlite3_finalize(statement);
        sqlite3_close(conn);
    }
}


bool Server_Session::get_paths() {
    sqlite3* conn;
    unsigned char *paths_ch;
    bool found = false;
    if (sqlite3_open("../Clients.sqlite", &conn) == SQLITE_OK) {
        std::string sqlStatement = std::string("SELECT paths FROM client WHERE username = '") + username + std::string("';");
        sqlite3_stmt *statement;
        if (sqlite3_prepare_v2(conn, sqlStatement.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
            while( sqlite3_step(statement) == SQLITE_ROW ) {
                paths_ch = const_cast<unsigned char*>(sqlite3_column_text(statement, 0));
            }
            if (paths_ch != NULL) {
                std::string paths_str(reinterpret_cast<char*>(paths_ch));   //cast in order to remove unsigned
                found = true;
                boost::property_tree::ptree pt;
                boost::property_tree::read_json(paths_str, pt);
                for (auto pair : pt) {
                    std::stringstream hash_stream(pair.second.data());
                    size_t hash;
                    hash_stream >> hash;
                    paths[pair.first] = hash;
                }
            }
        } else {
            server_availability = false;
            std::cout << "Database Connection Error" << std::endl;
        }
        sqlite3_finalize(statement);
        sqlite3_close(conn);
        return found;
    } else {
        server_availability = false;
    }
    return found;
}

std::vector<std::string> Server_Session::compare_paths(ptree client_pt) {
    std::vector<std::string> response_paths;
    for (auto &entry : client_pt) {
        auto it = paths.find(entry.first);
        if (it != paths.end()) {
            std::stringstream hash_stream(entry.second.data());
            size_t entry_hash;
            hash_stream >> entry_hash;
            if (it->second != entry_hash) {
                response_paths.emplace_back(it->first);
            }
        } else {
            response_paths.emplace_back(it->first);
        }
    }
    return response_paths;
}

void Server_Session::enqueue_msg(const Message &msg, bool close) {
    bool write_in_progress = !write_queue_s.empty();
    write_queue_s.push(msg);
    if (!write_in_progress) do_write();
    if (close) socket_.close();
}

void Server_Session::request_handler(Message msg) {
    //std::cout << *msg.get_msg_ptr() << std::endl;
    bool close = false;
    msg.decode_message();
    auto header = static_cast<action_type>(msg.get_header());
    std::string data = msg.get_data();
    int status_type;
    std::string response_str;
    Message response_msg;

    if (header != action_type::login && username.empty()) {

        status_type = 6;
        response_str = std::string("Login needed");
        close = true;

    } else {

        switch (header) {

            case (action_type::login) : {

                auto credentials = msg.get_credentials();
                bool found = Server_Session::check_database(std::get<0>(credentials), std::get<1>(credentials));
                if (found) {
                    username = std::get<0>(credentials);
                    status_type = 0;
                    response_str = std::string("Access granted");
                } else {
                    status_type = 6;
                    response_str = std::string("Access denied, try again");
                    close = true;
                }

                break;
            }

            case (action_type::synchronize) : {

                boost::property_tree::ptree pt;
                std::stringstream data_stream;
                data_stream << data;
                boost::property_tree::json_parser::read_json(data_stream, pt);
                if (get_paths()) {
                    // vede se il database risponde
                    if (server_availability) {
                        // deve confrontare le mappe e rispondere con in_need o no_need
                        std::vector<std::string> missing_paths = compare_paths(pt);
                        if (missing_paths.empty()) {
                            status_type = 4;
                            response_str = "No need";
                        } else {
                            status_type = 5;
                            for (const auto &path : missing_paths) response_str += path + "||";
                        }
                    } else {
                        status_type = 7;
                        response_str = "Service unavailable";
                    }
                } else {
                    // deve rispondere in_need con tutta la mappa ricevuta come dati
                    status_type = 5;
                    for (const auto &path : pt) response_str += path.first + "||";
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
                if (!boost::filesystem::is_directory(directory)) {
                    boost::filesystem::create_directory(directory);
                }
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
                bool isDirectory = pt.get<bool>("isDirectory");
                update_paths(path, hash);
                if (!isDirectory) {
                    auto content = pt.get<std::string>("content");
                    std::string relative_path =
                            std::string("../") + std::string(username) + std::string("/") +
                            std::string(path);
                    if (relative_path.find(':') < relative_path.size())
                        relative_path.replace(relative_path.find(':'), 1, ".");
                    boost::filesystem::remove(relative_path.data());
                    boost::filesystem::ofstream outFile(relative_path.data());
                    outFile.open(relative_path.data(), std::ios::binary);
                    outFile.write(content.data(), content.size());
                    outFile.close();
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
                remove_path(path);
                std::string directory = std::string("../../server/") + std::string(username);
                std::string relative_path = directory + std::string("/") + std::string(path);
                if (relative_path.find(':') < relative_path.size())
                    relative_path.replace(relative_path.find(':'), 1, ".");
                boost::filesystem::remove_all(relative_path.data());

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
    // creating response message
    response_msg.encode_header(status_type);
    response_msg.encode_data(response_str);
    response_msg.zip_message();
    enqueue_msg(response_msg, close);
}

Server_Session::~Server_Session() {
    //std::cout << "Distruttore server session" << std::endl;
    //if (!paths.empty()) update_db_paths();
}
