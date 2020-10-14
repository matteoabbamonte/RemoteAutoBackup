#include "Server_Session.h"

Server_Session::Server_Session(tcp::socket &socket) : socket_(std::move(socket)) {}

/*tcp::socket& Server_Session::socket() {
    return socket_;
}*/

void Server_Session::insert_path(std::string path, size_t hash) {
    paths[path] = hash;
}

void Server_Session::start() {
    do_read_size();
}

void Server_Session::do_read_size() {
    auto self(std::shared_ptr<Server_Session>(this));
    int size;
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.get_size_ptr(), sizeof(size)),
                            [this, self](boost::system::error_code ec, std::size_t /*length*/)
                            {
                                if (!ec && read_msg_.decode_size())
                                {
                                    do_read_body();
                                }
                                else
                                {
                                    std::cout << "Size is not a number" << std::endl;
                                }
                            });
}

void Server_Session::do_read_body() {
    auto self(std::shared_ptr<Server_Session>(this));
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.get_msg_ptr(), read_msg_.get_size_int()),
                            [this, self](boost::system::error_code ec, std::size_t /*length*/)
                            {
                                if (!ec)
                                {
                                    read_msg_.decode_message();
                                    action_type header = static_cast<action_type>(read_msg_.get_header());
                                    std::string data = read_msg_.get_data();
                                    //if header == login then read data, take username and password, check db and insert username in clients map
                                    if (header == action_type::login) {
                                        auto credentials = read_msg_.get_credentials();
                                        bool found = Server_Session::check_database(std::get<0>(credentials), std::get<1>(credentials));
                                        if (found) {
                                            commonSession->push_username(socket_, std::get<0>(credentials));
                                            std::cout << "found" << std::endl;
                                        }
                                        else {
                                            commonSession->delete_client(socket_);
                                            std::cout << "not found" << std::endl;
                                        }
                                    } else {
                                        std::string username = commonSession->get_username(socket_);
                                        commonSession->push_op(username, header, data, socket_);
                                    }
                                    do_read_size();
                                }
                                else {
                                    std::cout << ec.message() << std::endl;
                                }
                            });
}

void Server_Session::do_write() {
    auto self(std::shared_ptr<Server_Session>(this));
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
                                     std::cout << ec.message() << std::endl;
                                 }
                             });
}

bool Server_Session::check_database(std::string username, std::string password) {
    sqlite3* conn;
    int count = 0;
    if (sqlite3_open("Clients.sqlite", &conn) == SQLITE_OK) {
        std::string sqlStatement = "SELECT COUNT(*) FROM client WHERE username = '" + username
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

bool Server_Session::get_paths(std::string username) {
    sqlite3* conn;
    unsigned char *paths_ch;
    bool found = true;
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
        }
        else {
            std::cout << "Database Connection Error" << std::endl;
        }
        sqlite3_finalize(statement);
        sqlite3_close(conn);
        return found;
    }
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
