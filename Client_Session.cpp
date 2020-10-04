#include "Client_Session.h"

Client_Session::Client_Session(tcp::socket &socket, std::string username) : socket_(std::move(socket)), username(username) {}

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
                                    std::string header = read_msg_.get_header();
                                    std::string data = read_msg_.get_data();
                                    responsesQueue.push_operation(username, header, data);

                                    do_read_size();
                                }
                                else {
                                    std::cout << ec.message() << std::endl;
                                }
                            });
}