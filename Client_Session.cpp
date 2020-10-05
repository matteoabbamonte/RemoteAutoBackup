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
                                    status_type header = boost::lexical_cast<status_type>(read_msg_.get_header());
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

void Client_Session::enqueue_msg(const Message &msg) {
    write_queue_.emplace_back(msg);
}

