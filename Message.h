#pragma once

#include <cstdio>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

class Message {
    boost::property_tree::ptree pt;

public:

    void encode_action(std::string& action);

    void encode_data(std::string& data);

    std::vector<char> zip_message();

    //to be defined...

    void encode_action();

    void encode_data();

    /*bool decode_action() {
        std::stringstream stream;
        stream << input.get();
        boost::property_tree::ptree pt;
        boost::property_tree::json_parser::read_json(stream, pt);
        this->key = pt.get<std::string>("key");
        this->value = pt.get<V>("value");
        this->acc = pt.get<A>("acc");
    }*/

};