#include "Message.h"

Message::Message() {
    size = new char[4];
}



void Message::encode_header(std::string& header) {
    pt.put("header", header);
}

void Message::encode_data(std::string& data) {
    pt.put("data", data);
}

void Message::zip_message() {
    std::string message_str;
    boost::property_tree::json_parser::write_json(message_str, pt);
    message_str = std::to_string(message_str.size()) + message_str;
    //message = std::vector<char>(message_str.begin(), message_str.end());
}

char* Message::get_pointer() {
    return reinterpret_cast<char*>(&message[0]);
}

std::size_t Message::get_msg_length() {
    return message.size();
}

char* Message::get_size_ptr() {
    return size;
}

char*  Message::get_msg_ptr() {
    return msg_ptr;
}

bool Message::decode_size() {
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

std::string Message::get_header() {
    return pt.get<std::string>("header");
}

std::string Message::get_data() {
    return pt.get<std::string>("data");
}

std::tuple<std::string, std::string> Message::get_credentials() {
    auto credentials_str = pt.get<std::string>("data");
    int separator_pos = credentials_str.find("||");
    auto username = credentials_str.substr(0, separator_pos);
    auto password = credentials_str.substr(separator_pos+2);
    return std::tuple<std::string, std::string>(username, password);
}

Message::~Message() {
    delete [] size;
    delete [] msg_ptr;
}


