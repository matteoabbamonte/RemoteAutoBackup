#include "Client_Session.h"
#include <boost/lexical_cast.hpp>

Client_Session::Client_Session(tcp::socket &socket) : socket_(std::move(socket)) {}

tcp::socket& Client_Session::socket() {
    return socket_;
}

void Client_Session::do_read_size() {
    auto self(std::shared_ptr<Client_Session>(this));
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

void Client_Session::do_read_body() {
    auto self(std::shared_ptr<Client_Session>(this));
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.get_msg_ptr(), read_msg_.get_size_int()),
                            [this, self](boost::system::error_code ec, std::size_t /*length*/)
                            {
                                if (!ec)
                                {
                                    read_msg_.decode_message();
                                    status_type header = static_cast<status_type>(read_msg_.get_header());
                                    std::string data = read_msg_.get_data();
                                    responsesQueue.push_response(header, data);
                                    do_read_size();
                                }
                                else {
                                    std::cout << ec.message() << std::endl;
                                }
                            });
}

void Client_Session::do_write() {
    auto self(std::shared_ptr<Client_Session>(this));
    boost::asio::async_write(socket_,
                             boost::asio::buffer(write_queue_c.front().get_msg_ptr(),
                                                 write_queue_c.front().get_size_int()),
                             [this, self](boost::system::error_code ec, std::size_t /*length*/)
                             {
                                 if (!ec)
                                 {
                                     write_queue_c.pop_front();
                                     if (!write_queue_c.empty())
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

void Client_Session::enqueue_msg(const Message &msg) {
    write_queue_c.emplace_back(msg);
}

void Client_Session::get_credentials() {
    std::cout << "Insert username: ";
    std::getline(std::cin, this->username);
    std::string password;
    std::cout << "Insert password: ";
    std::getline(std::cin, password);
    Message write_msg_;
    write_msg_.put_credentials(this->username, password);
    write_msg_.encode_header(0);
    write_msg_.zip_message();
    enqueue_msg(write_msg_);
}

