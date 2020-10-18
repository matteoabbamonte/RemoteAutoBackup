#include "Client.h"

Client::Client(boost::asio::io_context& io_context, const tcp::resolver::results_type& endpoints) : io_context_(io_context), socket_(io_context), cs(new Client_Session(socket_))
{
    do_connect(endpoints);
    cs->get_credentials();

}

void Client::do_connect(const tcp::resolver::results_type& endpoints)
{
    boost::asio::async_connect(socket_, endpoints,
                               [this](boost::system::error_code ec, const tcp::endpoint&)
                               {
                                   if (!ec)
                                   {
                                       cs->do_read_size();
                                   }
                               });
}

void Client::send_actions(std::string path_to_watch, FileStatus status, bool isFile) {
    // Process only regular files, all other file types are ignored
    if(boost::filesystem::is_regular_file(boost::filesystem::path(path_to_watch)) || boost::filesystem::is_directory(boost::filesystem::path(path_to_watch)) || status == FileStatus::erased) {
        switch(status) {
            case FileStatus::created:
                if (isFile) {
                    std::cout << "File created: " << path_to_watch << '\n';
                } else std::cout << "Directory created: " << path_to_watch << '\n';
                break;
            case FileStatus::modified:
                if (isFile) {
                    std::cout << "File modified: " << path_to_watch << '\n';
                } else std::cout << "Directory modified: " << path_to_watch << '\n';
                break;
            case FileStatus::erased:
                if (isFile) {
                    std::cout << "File erased: " << path_to_watch << '\n';
                } else std::cout << "Directory erased: " << path_to_watch << '\n';
                break;
            default:
                std::cout << "Error! Unknown file status.\n";
        }
    } else return;

}
