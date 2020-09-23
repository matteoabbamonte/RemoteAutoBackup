#pragma once

#include <cstdio>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

class Message {
    boost::property_tree::ptree pt;
    std::vector<char> message;
    char *size = new char[4];   // 4B dim int
    char *msg_ptr;

public:

    void encode_action(std::string& action);

    void encode_data(std::string& data);

    void zip_message();

    char* get_pointer();

    std::size_t get_msg_length();

    char* get_size();

    char* get_msg_ptr();

    bool decode_size();

    void decode_message();

    std::string get_action();

    std::vector<char> get_data();

};