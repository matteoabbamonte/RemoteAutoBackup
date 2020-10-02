#include "Session.h"
#include <sqlite3.h>

Session::Session(boost::asio::io_context& io_context) : socket_(io_context) {}

boost::asio::ip::tcp::socket& Session::socket() {
    return socket_;
}

void Session::start(OperationsQueue & queue) {
    do_read_size(queue);
}

void Session::do_read_size(OperationsQueue & queue) {
    auto self(std::shared_ptr<Session>(this));
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.get_size_ptr(), sizeof(int)),
                            [this, self, &queue](boost::system::error_code ec, std::size_t /*length*/)
                            {
                                if (!ec && read_msg_.decode_size())
                                {
                                    do_read_body(queue);
                                }
                                else
                                {
                                    std::cout << "Size is not a number" << std::endl;
                                }
                            });
}

void Session::do_read_body(OperationsQueue & queue) {
    auto self(std::shared_ptr<Session>(this));
    std::tuple<std::string, std::string> action_data;
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.get_msg_ptr(), read_msg_.get_size_int()),
                            [this, self, &queue](boost::system::error_code ec, std::size_t /*length*/)
                            {
                                if (!ec)
                                {
                                    read_msg_.decode_message();
                                    std::string action = read_msg_.get_action();
                                    std::string data = read_msg_.get_data();
                                    //if action == login then read data, take username and password, check db and insert username in clients map
                                    if (action == "l") {
                                        auto credentials = read_msg_.get_credentials();
                                        bool found = Session::check_database(std::get<0>(credentials),std::get<1>(credentials));
                                        if (found) {
                                            
                                            std::cout << "found" << std::endl;
                                        }
                                        else
                                            std::cout << "not found" << std::endl;

                                    } else {
                                        //queue.push_operation();
                                    }
                                    //do_read_size();
                                }
                                else
                                {
                                    std::cout << ec.message() << std::endl;
                                }
                            });
}

void Session::do_write() {
    auto self(std::shared_ptr<Session>(this));
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

void Session::enqueue_msg(const Message &msg) {
    write_queue_.emplace_back(msg);
}

bool Session::check_database(std::string username, std::string password) {
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