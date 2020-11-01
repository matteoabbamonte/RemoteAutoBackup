#include <iostream>
#include "Message.h"

Message::Message() {
    //size = new char[11];
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
    //u_long dim = message_string.size();
    //std::string str_dim = std::to_string(dim);
    //str_dim.insert(str_dim.begin(), 10 - str_dim.length(), '0');
    //message_string = str_dim + message_string;
    msg_ptr = std::make_shared<std::string>(message_string);
    msg_ptr->resize(message_string.size());
    //strncpy(size, str_dim.c_str(), 10);
    //msg_ptr->insert(0, message_string);
    //strncpy(msg_ptr, message_string.c_str(), message_string.size());
}

/*char* Message::get_size_ptr() {
    return size;
}*/

/*std::shared_ptr<std::string> Message::get_msg_ptr(int size_b) {
    msg_ptr = std::make_shared<std::string>();
    msg_ptr->resize(size_b);
    return msg_ptr;
}*/

std::shared_ptr<std::string> Message::get_msg_ptr() {
    if (msg_ptr == nullptr) msg_ptr = std::make_shared<std::string>();
    return msg_ptr;
}

/*bool Message::decode_size() {
    size[10] = '\0';
    if (std::stoi(std::string(size)) > 0) {
        msg_ptr = std::make_shared<std::string>();
        //msg_ptr->resize(std::stoi(std::string(size)));
        return true;
    }
    return false;
}*/

int Message::get_size_int() {
    int size_b = std::stoi(std::string(size));
    return size_b;
}

void Message::decode_message() {
    msg_ptr->shrink_to_fit();
    std::stringstream stream;
    std::cout << *msg_ptr << std::endl;
    std::string string(*msg_ptr);
    stream << (*msg_ptr);
    std::cout << stream.str() << std::endl;
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

/*Message::~Message() {
    //delete [] size;
    //delete [] msg_ptr;
}*/

