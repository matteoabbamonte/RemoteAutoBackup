#include "Message.h"

void Message::encode_action(std::string& action) {
    pt.put("action", action);
}

void Message::encode_data(std::string& data) {
    pt.put("data", data);
}

std::vector<char> Message::zip_message() {
    std::string message;
    boost::property_tree::json_parser::write_json(message, pt);
    std::vector<char> result(message.begin(), message.end());
    return result;
}