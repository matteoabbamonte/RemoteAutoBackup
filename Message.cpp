#include <iostream>
#include "Message.h"

Message::Message() {
    msg_ptr = std::make_shared<std::string>();
}

void Message::encode_header(int header) {
    pt.add("header", header);
}

void Message::encode_data(std::string& data) {
    pt.add("data", data);
}

void Message::zip_message() {
    std::stringstream message_stream;
    boost::property_tree::json_parser::write_json(message_stream, pt);
    std::string message_string(message_stream.str());
    //boost::property_tree::ptree pt1;
    //read_json(message_stream, pt1);
    msg_ptr = std::make_shared<std::string>(message_string);
    msg_ptr->resize(message_string.size());
}

std::shared_ptr<std::string> Message::get_msg_ptr() {
    std::string temp(*msg_ptr);
    return msg_ptr;
}

void Message::decode_message() {
    //msg_ptr->shrink_to_fit();
    std::stringstream stream;
    stream << (*msg_ptr);
    //stream.seekg(0, stream.beg);
    std::string temp(stream.str());
    read_json(stream, pt);
    //boost::property_tree::json_parser::read_json(stream, pt);
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

void Message::clear() {
    msg_ptr = std::make_shared<std::string>();
}

/*Message::~Message() {
    //delete [] size;
    //delete [] msg_ptr;
}*/

