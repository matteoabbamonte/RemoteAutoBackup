#include "Session.h"

Session::Session(boost::asio::io_context& io_context) : socket_(io_context) {}

Session::pointer Session::create(boost::asio::io_context& io_context) {
    return Session::pointer(new Session(io_context));
}

boost::asio::ip::tcp::socket& Session::socket() {
    return socket_;
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
