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

    /// Allocating memory and constructing a msgPtr before the do_read calls a get_msg_ptr on it
    Message();

    /// Assigning the content of the json (pt) to the msgPtr
    int zip_message();

    /// Getting pointer to the message
    msg_ptr get_msg_ptr();

    /// Saving the content of the msgPtr to the json (pt)
    int decode_message();

    /// Getting the header of the message from the json (pt)
    int get_header();

    /// Getting the data of the message from the json (pt) in string format
    std::string get_str_data();

    /// Getting the data of the message from the json (pt) in json format
    boost::property_tree::ptree get_pt_data();

    /// Extracting and getting the credentials from the json (pt)
    std::tuple<std::string, std::string> get_credentials();

    /// Inserting credentials into the message json (pt)
    int put_credentials(const std::string& username, const std::string& password);

    /// 1st overload: Assembling the json that has to be saved in the final message (with string data)
    int encode_message(int header, const std::string& data);

    /// 2nd overload: Assembling the json that has to be saved in the final message (with json data)
    int encode_message(int header, const boost::property_tree::ptree& data);
};