//
// Created by matte on 21/09/2020.
//

#include "ConnectionHandler.h"

ConnectionHandler::ConnectionHandler(boost::asio::io_context& io_context) : socket_(io_context) {}

void ConnectionHandler::handle_write(const boost::system::error_code& /*error*/, size_t /*bytes_transferred*/) {}

ConnectionHandler::pointer ConnectionHandler::create(boost::asio::io_context& io_context) {
    return ConnectionHandler::pointer(new ConnectionHandler(io_context));
}

boost::asio::ip::tcp::socket& ConnectionHandler::socket() {
    return socket_;
}