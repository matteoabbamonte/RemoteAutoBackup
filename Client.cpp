#include "Client.h"

Client::Client(boost::asio::io_context& io_context, const tcp::resolver::results_type& endpoints) : io_context_(io_context), socket_(io_context), cs(new Client_Session(socket_))
{
    do_connect(endpoints);
    
}

void Client::do_connect(const tcp::resolver::results_type& endpoints)
{
    boost::asio::async_connect(socket_, endpoints,
                               [this](boost::system::error_code ec, tcp::endpoint&)
                               {
                                   if (!ec)
                                   {
                                       cs->do_read_size();
                                   }
                               });
}
