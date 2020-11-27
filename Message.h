#pragma once

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/exceptions.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <tuple>

typedef std::shared_ptr<std::string> msg_ptr;

class Message {
    boost::property_tree::ptree pt;
    msg_ptr msgPtr;

public:

    // Allocating memory and constructing a msgPtr before the do_read calls a get_msg_ptr on it
    Message();

    // Assigning the content of the json (pt) to the msgPtr
    void zip_message();

    // Getting pointer to the message
    msg_ptr get_msg_ptr();

    // Saving the content of the msgPtr to the json (pt)
    void decode_message();

    // Getting the header of the message from the json (pt)
    int get_header();

    // Getting the data of the message from the json (pt)
    std::string get_data();

    // Extracting and getting the credentials from the json (pt)
    std::tuple<std::string, std::string> get_credentials();

    // Inserting credentials into the message json (pt)
    void put_credentials(const std::string& username, const std::string& password);

    // Assembling the json that has to be saved in the final message
    void encode_message(int header, std::string& data);
};