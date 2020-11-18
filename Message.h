#pragma once

#include <cstdio>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/exceptions.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <tuple>

class Message {
    boost::property_tree::ptree pt;
    std::shared_ptr<std::string> msg_ptr;

public:
    Message();

    void zip_message();

    void clear();

    std::shared_ptr<std::string> get_msg_ptr();    //get pointer to the beginning of the message buffer

    void decode_message();

    int get_header();

    std::string get_data();

    std::tuple<std::string, std::string> get_credentials();

    void put_credentials(const std::string& username, const std::string& password);

    void encode_message(int header, std::string& data);
};