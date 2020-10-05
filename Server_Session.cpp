#include "Server_Session.h"

void Common_Session::push(tcp::socket &socket) {
    clients[socket] = "";
}

void Common_Session::push_username (tcp::socket &socket, const std::string username) {
    auto ptr_user = clients.find(socket);
    if (ptr_user != clients.end()) {
        clients[socket] = username;
    }
}

void Common_Session::delete_client(tcp::socket &socket) {
    clients.erase(socket);
}

std::string Common_Session::get_username(tcp::socket &socket) {
    return clients[socket];
}

Common_Session::Common_Session() {}

Server_Session::Server_Session(tcp::socket &socket) : socket_(std::move(socket)) {}

tcp::socket& Server_Session::socket() {
    return socket_;
}

void Server_Session::start() {
    do_read_size();
}

void Server_Session::do_read_size() {
    auto self(std::shared_ptr<Server_Session>(this));
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.get_size_ptr(), sizeof(int)),
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
                                    std::string header = read_msg_.get_header();
                                    std::string data = read_msg_.get_data();
                                    //if header == login then read data, take username and password, check db and insert username in clients map
                                    if (header == "l") {
                                        auto credentials = read_msg_.get_credentials();
                                        bool found = Server_Session::check_database(std::get<0>(credentials), std::get<1>(credentials));
                                        if (found) {
                                            push_username(socket_, std::get<0>(credentials));
                                            std::cout << "found" << std::endl;
                                        }
                                        else {
                                            delete_client(socket_);
                                            std::cout << "not found" << std::endl;
                                        }
                                    } else {
                                        std::string username = get_username(socket_);
                                        operationsQueue.push_operation(username, header, data);
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
                             boost::asio::buffer(write_queue_.front().get_msg_ptr(),
                                                 write_queue_.front().get_size_int()),
                             [this, self](boost::system::error_code ec, std::size_t /*length*/)
                             {
                                 if (!ec)
                                 {
                                     write_queue_.pop_front();
                                     if (!write_queue_.empty())
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

void Server_Session::enqueue_msg(const Message &msg) {
    write_queue_.emplace_back(msg);
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