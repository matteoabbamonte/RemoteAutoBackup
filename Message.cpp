#include <iostream>
#include "Message.h"

Message::Message() {
    size = new char[4];
    //msg_ptr = nullptr;
}

void Message::encode_header(int header) {
    pt.put("header", header);
}

void Message::encode_data(std::string& data) {
    pt.put("data", data);
}

void Message::zip_message() {
    std::stringstream message_stream;
    boost::property_tree::json_parser::write_json(message_stream, pt);
    auto message_string = message_stream.str();
    message_string = std::to_string(message_string.size()) + std::string(message_string);
    //message = std::vector<char>(message_string.begin(), message_string.end());
    msg_ptr = new char[message_string.size()+1];
    strcpy(msg_ptr, message_string.c_str());
}

/*
char* Message::get_pointer() {
    return reinterpret_cast<char*>(&message[0]);
}

std::size_t Message::get_msg_length() {
    return message.size();
}
*/

char* Message::get_size_ptr() {
    return size;
}

char* Message::get_msg_ptr() {
    return msg_ptr;
}

bool Message::decode_size() {
    size[sizeof(int)] = '\n';
    if (std::atoi(size)) {
        msg_ptr = new char[std::atoi(size)+1];
        return true;
    }
    return false;
}

int Message::get_size_int() {
    return std::atoi(size);
}

void Message::decode_message() {
    std::stringstream stream;
    stream << msg_ptr;
    boost::property_tree::json_parser::read_json(stream, pt);
}

int Message::get_header() {
    return pt.get<int>("header");
}

std::string Message::get_data() {
    return pt.get<std::string>("data");
}

std::tuple<std::string, std::string> Message::get_credentials() {
    auto credentials_str = pt.get<std::string>("data");
    int separator_pos = credentials_str.find("||");
    auto username = credentials_str.substr(0, separator_pos);
    auto password = credentials_str.substr(separator_pos+2);
    return std::make_tuple(username, password);
}

void Message::put_credentials(const std::string& username, const std::string& password) {
    std::string user_pass = std::string(username) + std::string("||") + std::string(password);
    encode_data(user_pass);
}

Message::~Message() {
    delete [] size;
    //delete [] msg_ptr;
}

