#include <boost/asio/thread_pool.hpp>
#include "Backup_Server.h"

void Backup_Server::do_accept() {
    acceptor.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket)
            {
                if (!ec)
                {
                    clients[socket] = pClient(new Client());
                    //std::make_shared<chat_session>(std::move(socket), room_)->start();
                }
                do_accept();
            });
}

Backup_Server::Backup_Server(boost::asio::io_context& io_context, const tcp::endpoint& endpoint) : acceptor(io_context, endpoint) {
    boost::asio::thread_pool pool;
    boost::asio::post(pool, [this](){
        while (true) {
            for (auto it = clients.begin(); it != clients.end(); it++) {

            }
            break;
        }
    });
    do_accept();
}
