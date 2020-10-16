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
            Message response_msg;

            switch (header) {

                case(action_type::synchronize) : {

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

                    std::string path = pt.get<std::string>("path");
                    std::size_t hash = pt.get<std::size_t>("hash");
                    bool isDirectory = pt.get<bool>("isDirectory");
                    serverSession.update_paths(path, hash);
                    std::string relative_path = std::string("../") + std::string(username) + std::string("/") + std::string(path);
                    if (isDirectory) {
                        //create a directory with the specified name
                        boost::filesystem::create_directory(relative_path);
                    } else {
                        //create a file with the specified name
                        std::string content = pt.get<std::string>("content");
                        if (relative_path.find(':') < relative_path.size()) relative_path.replace(relative_path.find(':'), 1, ".");
                        boost::filesystem::ofstream outFile(relative_path.data());
                        if (!content.empty()) {
                            outFile.open(relative_path.data(), std::ios::binary);
                            outFile.write(content.data(), content.size());
                            outFile.close();
                        }
                    }
                    break;
                }

                case(action_type::update) : {

                    std::string path = pt.get<std::string>("path");
                    std::size_t hash = pt.get<std::size_t>("hash");
                    bool isDirectory = pt.get<bool>("isDirectory");
                    serverSession.update_paths(path, hash);
                    if (!isDirectory) {
                        std::string content = pt.get<std::string>("content");
                        std::string relative_path = std::string("../") + std::string(username) + std::string("/") + std::string(path);
                        if (relative_path.find(':') < relative_path.size()) relative_path.replace(relative_path.find(':'), 1, ".");
                        boost::filesystem::ofstream outFile;
                        outFile.open(relative_path.data(), std::ios::binary);
                        //overwrite file content with new content
                    }

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
