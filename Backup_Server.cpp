#include <boost/asio/thread_pool.hpp>
#include "Backup_Server.h"

void Backup_Server::do_accept() {
    acceptor.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket)
            {
                if (!ec) {
                    std::make_shared<Server_Session>(socket)->start();
                }
                do_accept();
            });
}

Backup_Server::Backup_Server(boost::asio::io_context& io_context, const tcp::endpoint& endpoint) : acceptor(io_context, endpoint), active(true) {
    boost::asio::thread_pool pool;
    do_accept();
    boost::asio::post(pool, [this](){
        while (active) {
            auto operation = commonSession.pop_op();
            std::string username = operation.username;
            auto header = static_cast<action_type>(operation.header);
            std::string data = operation.data;
            tcp::socket& socket = operation.socket;
            Server_Session serverSession(socket);
            Message response_msg;

            switch (header) {

                case(action_type::synchronize) : {

                    boost::property_tree::ptree pt;
                    boost::property_tree::json_parser::read_json(data, pt);
                    if (serverSession.get_paths(username)) {
                        //deve confrontare le mappe e rispondere con in_need o no_need
                        std::vector<std::string> missing_paths = serverSession.compare_paths(pt);
                        if (missing_paths.empty()) {
                            response_msg.encode_header(4);
                            std::string no_need_str = "no_need";
                            response_msg.encode_data(no_need_str);
                        } else {
                            response_msg.encode_header(5);
                            std::string need_str;
                            for (const auto & path : missing_paths) need_str += path + "||";
                            response_msg.encode_data(need_str);
                        }
                    } else {
                        //deve rispondere in_need con tutta la mappa ricevuta come dati
                        response_msg.encode_header(5);
                        std::string need_str;
                        for (const auto & path : pt) need_str += path.first + "||";
                        response_msg.encode_data(need_str);
                    }
                    response_msg.zip_message();
                    serverSession.enqueue_msg(response_msg);
                    break;
                }

                case(action_type::create) : {

                    boost::property_tree::ptree pt;
                    boost::property_tree::json_parser::read_json(data, pt);
                    std::string path = pt.get<std::string>("path");
                    std::size_t hash = pt.get<std::size_t>("hash");
                    bool isDirectory = pt.get<bool>("isDirectory");
                    serverSession.insert_path(path, hash);
                    std::string relative_path = std::string(username) + std::string("/") + std::string(path);
                    if (isDirectory) {
                        //create a directory with the specified name
                        //boost::filesystem::create_directory
                    } else {
                        //create a file with the specified name
                    }
                    break;
                }

                case(action_type::update) : {
                    break;
                }

                case(action_type::erase) : {
                    break;
                }
            }
            // !!!!
            break;
        }
    });
    
}
