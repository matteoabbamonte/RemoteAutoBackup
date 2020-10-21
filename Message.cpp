#include <iostream>
#include "Message.h"

Message::Message() {
    size = new char[10];
    msg_ptr = nullptr;
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
    std::string message_string = message_stream.str();
    std::string str_dim = std::to_string(std::strlen(message_string.c_str()));
    str_dim.insert(str_dim.begin(), 10 - str_dim.length(), '0');
    message_string = str_dim + message_string;
    msg_ptr = new char[message_string.size()+1];
    strcpy(size, str_dim.c_str());
    size[10] = '\0';
    strcpy(msg_ptr, message_string.c_str());
    msg_ptr[message_string.size()] = '\0';
}

char* Message::get_size_ptr() {
    return size;
}

char* Message::get_msg_ptr() {
    return msg_ptr;
}

bool Message::decode_size() {
    size[10] = '\0';
    if (std::stoi(std::string(size)) > 0) {
        msg_ptr = new char[std::stoi(std::string(size))+1];
        return true;
    }
    return false;
}

int Message::get_size_int() {
    return std::stoi(std::string(size));
}

void Message::decode_message() {
    std::stringstream stream;
    msg_ptr[get_size_int()] = '\0';
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
    //delete [] size;
    //delete [] msg_ptr;
}

