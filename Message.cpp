#include "Message.h"

void Message::encode_action(std::string& action) {
    pt.put("action", action);
}

void Message::encode_data(std::string& data) {
    pt.put("data", data);
}

void Message::zip_message() {
    std::string message_str;
    boost::property_tree::json_parser::write_json(message_str, pt);
    message_str = std::to_string(message_str.size()) + message_str;
    message = std::vector<char>(message_str.begin(), message_str.end());
}

char* Message::get_pointer() {
    return reinterpret_cast<char*>(&message[0]);
}

std::size_t Message::get_msg_length() {
    return message.size();
}

char* Message::get_size() {
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

void Message::decode_message() {
    std::stringstream stream;
    stream << msg_ptr;
    boost::property_tree::json_parser::read_json(stream, pt);
}

std::string Message::get_action() {
    return pt.get<std::string>("action");
}

std::vector<char> Message::get_data() {
    return pt.get<std::vector<char>>("data");
}

