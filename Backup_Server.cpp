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
            boost::property_tree::ptree pt;
            boost::property_tree::json_parser::read_json(data, pt);
            Server_Session serverSession(socket);
            int status_type;
            std::string response_str;
            Message response_msg;

            if (!username.empty()) {

                switch (header) {

                    case(action_type::synchronize) : {

                        if (serverSession.get_paths()) {
                            // vede se il database risponde
                            if (serverSession.server_availability) {
                                //deve confrontare le mappe e rispondere con in_need o no_need
                                std::vector<std::string> missing_paths = serverSession.compare_paths(pt);
                                if (missing_paths.empty()) {
                                    status_type = 4;
                                    response_str = "No need";
                                } else {
                                    status_type = 5;
                                    for (const auto & path : missing_paths) response_str += path + "||";
                                }
                            } else {
                                status_type = 7;
                                response_str = "Service unavailable";
                            }
                        } else {
                            //deve rispondere in_need con tutta la mappa ricevuta come dati
                            status_type = 5;
                            for (const auto & path : pt) response_str += path.first + "||";
                        }

                        break;
                    }

                    case(action_type::create) : {

                        auto path = pt.get<std::string>("path");
                        auto hash = pt.get<std::size_t>("hash");
                        bool isDirectory = pt.get<bool>("isDirectory");
                        serverSession.update_paths(path, hash);
                        std::string relative_path = std::string("../") + std::string(username) + std::string("/") + std::string(path);
                        if (isDirectory) {
                            //create a directory with the specified name
                            boost::filesystem::create_directory(relative_path);
                        } else {
                            //create a file with the specified name
                            auto content = pt.get<std::string>("content");
                            if (relative_path.find(':') < relative_path.size())
                                relative_path.replace(relative_path.find(':'), 1, ".");
                            boost::filesystem::ofstream outFile(relative_path.data());
                            if (!content.empty()) {
                                outFile.open(relative_path.data(), std::ios::binary);
                                outFile.write(content.data(), content.size());
                                outFile.close();
                            }
                        }

                        status_type = 1;
                        response_str = std::string(path) + std::string("created");

                        break;
                    }

                    case(action_type::update) : {

                        auto path = pt.get<std::string>("path");
                        auto hash = pt.get<std::size_t>("hash");
                        bool isDirectory = pt.get<bool>("isDirectory");
                        serverSession.update_paths(path, hash);
                        if (!isDirectory) {
                            auto content = pt.get<std::string>("content");
                            std::string relative_path = std::string("../") + std::string(username) + std::string("/") + std::string(path);
                            if (relative_path.find(':') < relative_path.size()) relative_path.replace(relative_path.find(':'), 1, ".");
                            boost::filesystem::remove(relative_path.data());
                            boost::filesystem::ofstream outFile(relative_path.data());
                            outFile.open(relative_path.data(), std::ios::binary);
                            outFile.write(content.data(), content.size());
                            outFile.close();
                        }

                        status_type = 2;
                        response_str = std::string(path) + std::string("updated");
                        break;
                    }

                    case(action_type::erase) : {

                        auto path = pt.get<std::string>("path");
                        serverSession.remove_path(path);
                        std::string relative_path = std::string("../") + std::string(username) + std::string("/") + std::string(path);
                        if (relative_path.find(':') < relative_path.size())
                            relative_path.replace(relative_path.find(':'), 1, ".");
                        boost::filesystem::remove(relative_path.data());

                        status_type = 3;
                        response_str = std::string(path) + std::string("erased");
                        break;
                    }

                    default : {
                        status_type = 8;
                        response_str = std::string("Wrong action type");
                    }
                }
            } else {
                status_type = 6;
                response_str = std::string("Unauthorized access");
            }

            // creating response message
            response_msg.encode_header(status_type);
            response_msg.encode_data(response_str);
            response_msg.zip_message();
            serverSession.enqueue_msg(response_msg);
        }
    });
    
}
