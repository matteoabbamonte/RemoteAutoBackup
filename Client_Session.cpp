#include "Client_Session.h"
#include <boost/lexical_cast.hpp>
#include<boost/filesystem.hpp>

Client_Session::Client_Session(tcp::socket &socket) : socket_(std::move(socket)) {
    create_log_file();
}

void Client_Session::create_log_file() {
    boost::filesystem::ofstream("../../log.txt");
}

void Client_Session::do_read_size() {
    auto self(std::shared_ptr<Client_Session>(this));
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

void Client_Session::do_read_body() {
    auto self(std::shared_ptr<Client_Session>(this));
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.get_msg_ptr(), read_msg_.get_size_int()),
                            [this, self](boost::system::error_code ec, std::size_t /*length*/)
                            {
                                if (!ec)
                                {
                                    read_msg_.decode_message();
                                    //change header to status
                                    auto header = static_cast<status_type>(read_msg_.get_header());
                                    std::string data = read_msg_.get_data();
                                    //statusQueue.push_status(header, data);
                                    status_handler(header, data);
                                    do_read_size();
                                }
                                else {
                                    std::cout << ec.message() << std::endl;
                                }
                            });
}

void Client_Session::status_handler(int status, std::string data) {
    boost::filesystem::ofstream outFile;
    outFile.open("../../log.txt", std::ios::app);
    data.append("\n");
    outFile.write(data.data(), data.size());
    outFile.close();
    if (status_type::in_need) {
        // copiare il comando che faremo nello switch del main
    }
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
    do_write();
}

void Client_Session::get_credentials() {
    std::cout << "Insert username: ";
    std::getline(std::cin, this->username);
    std::string password;
    std::cout << "Insert password: ";
    std::getline(std::cin, password);
    Message write_msg;
    write_msg.put_credentials(this->username, password);
    write_msg.encode_header(0);
    write_msg.zip_message();
    enqueue_msg(write_msg);
}



