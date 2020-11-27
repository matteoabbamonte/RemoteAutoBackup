#include <iostream>
#include "Message.h"

Message::Message() {
    msgPtr = std::make_shared<std::string>();
}

void Message::zip_message() {
    try {
        std::stringstream message_stream;
        boost::property_tree::write_json(message_stream, pt);  // Saving the json in a stream
        msgPtr = std::make_shared<std::string>(message_stream.str());
    } catch (const boost::property_tree::ptree_error &err) {
        throw;
    }
}

std::shared_ptr<std::string> Message::get_msg_ptr() {
    return msgPtr;
}

void Message::decode_message() {
    try {
        std::stringstream stream;
        stream << (*msgPtr);
        std::cout << (*msgPtr) << std::endl;
        boost::property_tree::read_json(stream, pt);    // Re-creating json from data stream
    } catch (const boost::property_tree::ptree_error &err) {
        throw;
    }
}

int Message::get_header() {
    try {
        return pt.get<int>("header");
    } catch (const boost::property_tree::ptree_error &err) {
        throw;
    }
}

std::string Message::get_data() {
    try {
        return pt.get<std::string>("data");
    } catch (const boost::property_tree::ptree_error &err) {
        throw;
    }
}

std::tuple<std::string, std::string> Message::get_credentials() {
    try {
        auto credentials_str = pt.get<std::string>("data");
        int separator_pos = credentials_str.find("||");
        auto username = credentials_str.substr(0, separator_pos);
        auto password = credentials_str.substr(separator_pos+2);
        return std::make_tuple(username, password);
    } catch (const boost::property_tree::ptree_error &err) {
        throw;
    }
}

void Message::put_credentials(const std::string& username, const std::string& password) {
    try {
        std::string user_pass = std::string(username) + std::string("||") + std::string(password);
        encode_message(0, user_pass);
    } catch (const boost::property_tree::ptree_error &err) {
        throw;
    }
}

void Message::encode_message(int header, std::string& data) {
    try {
        pt.add("header", header);
        pt.add("data", data);
        zip_message();
    } catch (const boost::property_tree::ptree_error &err) {
        throw;
    }
}


