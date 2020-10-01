//
// Created by matte on 01/10/2020.
//

#include "Backup_Server.h"

void Backup_Server::do_accept() {
    acceptor.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket)
            {
                if (!ec)
                {
                    //std::make_shared<chat_session>(std::move(socket), room_)->start();
                }

                do_accept();
            });
}

Backup_Server::Backup_Server(boost::asio::io_context& io_context, const tcp::endpoint& endpoint) : acceptor(io_context, endpoint) {

}
