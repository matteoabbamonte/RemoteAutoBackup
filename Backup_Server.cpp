#include <boost/asio/thread_pool.hpp>
#include "Backup_Server.h"

void Backup_Server::do_accept() {
    acceptor.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket)
            {
                if (!ec) {
                    commonSession.push(socket);
                    std::make_shared<Server_Session>(socket)->start();
                }
                do_accept();
            });
}

Backup_Server::Backup_Server(boost::asio::io_context& io_context, const tcp::endpoint& endpoint) : acceptor(io_context, endpoint) {
    //commonSession = std::shared_ptr<Common_Session>(new Common_Session());
    boost::asio::thread_pool pool;
    boost::asio::post(pool, [this](){
        while (true) {
            auto operation = commonSession.pop_op();
            std::string username = operation.username;
            action_type header = static_cast<action_type>(operation.header);
            std::string data = operation.data;
            tcp::socket& socket = operation.socket;
            Server_Session* serverSession = dynamic_cast<Server_Session*>(commonSession); // downcast

            //Server_Session serverSession(socket);
            switch (header) {
                case(action_type::synchronize) : {
                    if (serverSession->get_paths()) {

                    }

                    break;
                }
                case(action_type::create) : {
                    break;
                }
                case(action_type::update) : {
                    break;
                }
                case(action_type::erase) : {
                    break;
                }
            }
            break;
        }
    });
    do_accept();
}
