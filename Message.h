#pragma once

#include <cstdio>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <tuple>

class Message {
    boost::property_tree::ptree pt;
    char *size;
    std::shared_ptr<std::string> msg_ptr;

public:
    Message();

    /*~Message();*/

    void encode_header(int header);

    void encode_data(std::string& data);

    void zip_message();

    void clear();

    char* get_size_ptr();   //get pointer to the beginning of the size buffer

    std::shared_ptr<std::string> get_msg_ptr();    //get pointer to the beginning of the message buffer

    void decode_message();

    int get_header();

    std::string get_data();

    std::tuple<std::string, std::string> get_credentials();

    void put_credentials(const std::string& username, const std::string& password);
};