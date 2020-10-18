#pragma once

#include <cstdio>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

class Message {
    boost::property_tree::ptree pt;
    std::vector<char> message;
    char *size;   // 4B dim int
    char *msg_ptr;

public:
    Message();

    virtual ~Message();

    void encode_header(int header);

    void encode_data(std::string& data);

    void zip_message();

    char* get_pointer();

    std::size_t get_msg_length();

    char* get_size_ptr();   //get pointer to the beginning of the size buffer

    char* get_msg_ptr();    //get pointer to the beginning of the message buffer

    int get_size_int();     //get the size as int

    bool decode_size();

    void decode_message();

    int get_header();

    std::string get_data();

    boost::tuple<std::string, std::string> get_credentials();

    void put_credentials(std::string username, std::string password);
};