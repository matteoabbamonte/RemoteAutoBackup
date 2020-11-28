#pragma once

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include <map>
#include <sqlite3.h>
#include <string>

class Database_Connection {
    std::string db_name;

public:

    Database_Connection();

    /// Looks for the received credentials and returns two booleans representing
    /// the presence (or the absence) of the entry and the availability of the database
    std::tuple<bool, bool> check_database(const std::string& temp_username, const std::string& password);

    /// Given a username, it saves in the given map the paths taken from the db, returns two booleans
    /// representing the presence (or the absence) of the entry and the availability of the database
    std::tuple<bool, bool> get_paths(std::map<std::string, std::size_t> &paths, const std::string& username);

    /// Given a username it saves the paths contained in the map to the corresponding field in the db
    /// and returns the availability of the database
    bool update_db_paths(std::map<std::string, std::size_t> &paths, const std::string& username);
};

