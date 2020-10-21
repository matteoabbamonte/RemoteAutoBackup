#include "Server_Session.h"

Server_Session::Server_Session(tcp::socket &socket) : socket_(std::move(socket)), server_availability(true) {}

void Server_Session::update_paths(std::string path, size_t hash) {
    paths[path] = hash;
}

void Server_Session::remove_path(std::string path) {
    paths.erase(path);
}

void Server_Session::start() {
    do_read_size();
}

void Server_Session::do_read_size() {
    auto self(shared_from_this());
    /*
    socket_.async_read_some(boost::asio::buffer(read_msg_.get_size_ptr(), sizeof(int)),
                            [this, self](boost::system::error_code ec, std::size_t /*length)
                            {
                                if (!ec && read_msg_.decode_size()) {
                                    do_read_body();
                                } else {
                                    std::cout << read_msg_.get_size_ptr() << std::endl;
                                    //std::cout << read_msg_.get_msg_ptr() << std::endl;
                                    std::cout << "Error inside do_read_size: ";
                                    std::cerr << ec.message() << std::endl;
                                    //std::cout << "Size is not a number" << std::endl;
                                }
                            });
    */
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.get_size_ptr(), 10),
                            [this, self](boost::system::error_code ec, std::size_t /*length*/)
                            {
                                if (!ec && read_msg_.decode_size()) {
                                    std::cout << "Size decoded, reading body" << std::endl;
                                    do_read_body();
                                } else {
                                    //std::cout << read_msg_.get_size_int() << std::endl;
                                    //std::cout << read_msg_.get_msg_ptr() << std::endl;
                                    std::cout << "Error inside do_read_size: ";
                                    std::cerr << ec.message() << std::endl;
                                    //std::cout << "Size is not a number" << std::endl;
                                }
                            });

}

void Server_Session::do_read_body() {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.get_msg_ptr(), read_msg_.get_size_int()),
                            [this, self](boost::system::error_code ec, std::size_t /*length*/)
                            {
                                if (!ec)
                                {
                                    std::cout << "Prima della decode" << std::endl;
                                    read_msg_.decode_message();
                                    std::cout << "Dopo la decode" << std::endl;
                                    action_type header = static_cast<action_type>(read_msg_.get_header());
                                    std::string data = read_msg_.get_data();
                                    //if header == login then read data, take username and password, check db and insert username in clients map
                                    if (header == action_type::login) {
                                        //Preparing response message syntax
                                        int status_type;
                                        std::string response_str;
                                        Message response_msg;

                                        auto credentials = read_msg_.get_credentials();
                                        bool found = Server_Session::check_database(std::get<0>(credentials), std::get<1>(credentials));
                                        if (found) {
                                            username = std::get<0>(credentials);
                                            status_type = 0;
                                            response_str = std::string("Access granted");
                                        } else {
                                            status_type = 6;
                                            response_str = std::string("Access denied, try again");
                                        }
                                        response_msg.encode_header(status_type);
                                        response_msg.encode_data(response_str);
                                        response_msg.zip_message();
                                        enqueue_msg(response_msg);
                                    } else {
                                        commonSession->push_op(username, header, data, socket_);
                                    }
                                    do_read_size();
                                } else {
                                    std::cout << "Error inside do_read_body: ";
                                    std::cout << ec.message() << std::endl;
                                }
                            });
}

void Server_Session::do_write() {
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
                             boost::asio::buffer(write_queue_s.front().get_msg_ptr(),
                                                 write_queue_s.front().get_size_int()),
                             [this, self](boost::system::error_code ec, std::size_t /*length*/)
                             {
                                 if (!ec)
                                 {
                                     write_queue_s.pop_front();
                                     if (!write_queue_s.empty())
                                     {
                                         do_write();
                                     }
                                 }
                                 else
                                 {
                                     std::cout << "Error inside do_write: ";
                                     std::cout << ec.message() << std::endl;
                                 }
                             });
}

bool Server_Session::check_database(std::string temp_username, std::string password) {
    sqlite3* conn;
    int count = 0;
    if (sqlite3_open("Clients.sqlite", &conn) == SQLITE_OK) {
        std::string sqlStatement = "SELECT COUNT(*) FROM client WHERE username = '" + temp_username
                + "' AND password = '" + password + "';";
        sqlite3_stmt *statement;

        if (sqlite3_prepare_v2(conn, sqlStatement.c_str(), -1, &statement, NULL) == SQLITE_OK) {
            while( sqlite3_step(statement) == SQLITE_ROW ) {
                count = sqlite3_column_int(statement, 0);
            }
        }
        else {
            std::cout << "Database Connection Error" << std::endl;
        }
        sqlite3_finalize(statement);
        sqlite3_close(conn);
    }
    return count;
}

bool Server_Session::get_paths() {
    sqlite3* conn;
    unsigned char *paths_ch;
    bool found = false;
    if (sqlite3_open("Clients.sqlite", &conn) == SQLITE_OK) {
        std::string sqlStatement = std::string("SELECT paths FROM client WHERE username = '") + username + std::string("';");
        sqlite3_stmt *statement;

        if (sqlite3_prepare_v2(conn, sqlStatement.c_str(), -1, &statement, NULL) == SQLITE_OK) {
            while( sqlite3_step(statement) == SQLITE_ROW ) {
                paths_ch = const_cast<unsigned char*>(sqlite3_column_text(statement, 0));
            }
            if (paths_ch == NULL) {
                found = false;
            } else {
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
            //std::cout << "Database Connection Error" << std::endl;
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

void Server_Session::enqueue_msg(const Message &msg) {
    write_queue_s.emplace_back(msg);
    do_write();
}
