#pragma once

#include <sqlite3.h>
#include <string>
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <map>

class Database_Connection {
    sqlite3* conn;
    std::string db_name;

public:

    Database_Connection();

    std::tuple<bool, bool> check_database(const std::string& temp_username, const std::string& password);

    bool update_db_paths(std::map<std::string, std::size_t> &paths, std::string username);

    std::tuple<bool, bool> get_paths(std::map<std::string, std::size_t> &paths, std::string username);
};

