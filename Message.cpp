#include <iostream>
#include "Message.h"

Message::Message() {
    msgPtr = std::make_shared<std::string>();
}

int Message::zip_message() {
    int res;
    try {
        std::stringstream message_stream;
        boost::property_tree::write_json(message_stream, pt);  // Saving the json in a stream
        msgPtr = std::make_shared<std::string>(message_stream.str());
        res = 1;
    } catch (const boost::property_tree::ptree_error &err) {
        res = 0;
    } catch (const std::bad_alloc &err) {
        res = 0;
    }
    return res;
}

std::shared_ptr<std::string> Message::get_msg_ptr() {
    return msgPtr;
}

int Message::decode_message() {
    int res;
    try {
        std::stringstream stream;
        stream << (*msgPtr);
        std::cout << (*msgPtr) << std::endl;
        boost::property_tree::read_json(stream, pt);    // Re-creating json from data stream
        res = 1;
    } catch (const boost::property_tree::ptree_error &err) {
        res = 0;
    }
}

int Message::get_header() {
    int res;
    try {
        res = pt.get<int>("header");
    } catch (const boost::property_tree::ptree_error &err) {
        res = 999;
    }
    return res;
}

std::string Message::get_data() {
    std::string res;
    try {
        res = pt.get<std::string>("data");
    } catch (const boost::property_tree::ptree_error &err) {
        res = "";
    }
    return res;
}

std::tuple<std::string, std::string> Message::get_credentials() {
    try {
        auto credentials_str = pt.get<std::string>("data");
        int separator_pos = credentials_str.find("||");
        auto username = credentials_str.substr(0, separator_pos);
        std::stringstream pwd_stream(credentials_str.substr(separator_pos+2));
        std::string pwd_hash;
        pwd_stream >> pwd_hash;
        return std::make_tuple(username, pwd_hash);
    } catch (const boost::property_tree::ptree_error &err) {
        throw;
    }
}

int Message::put_credentials(const std::string& username, const std::string& password) {
    int res;
    try {
        std::string user_pass = std::string(username) + std::string("||") + std::string(password);
        encode_message(0, user_pass);
        res = 1;
    } catch (const boost::property_tree::ptree_error &err) {
        res = 0;
    }
    return res;
}

int Message::encode_message(int header, std::string& data) {
    int res;
    try {
        pt.add("header", header);
        pt.add("data", data);
        zip_message();
        res = 1;
    } catch (const boost::property_tree::ptree_error &err) {
        res = 0;
    }
    return res;
}


