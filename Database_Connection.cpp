#include "Database_Connection.h"

Database_Connection::Database_Connection(): db_name("../Clients.sqlite") {}

std::tuple<bool, bool> Database_Connection::check_database(const std::string& username, const std::string& password) {
    std::cout << "Checking Database..." << std::endl;
    sqlite3* conn;  // Database handle defined by the sqlite3 structure
    int count = 0;
    bool db_availability = true;
    if (sqlite3_open(db_name.data(), &conn) == SQLITE_OK) {
        std::string sqlStatement = std::string("SELECT COUNT(*) FROM Client WHERE username = '") + username + std::string("' AND password = '") + password + std::string("';");
        sqlite3_stmt *statement;    // Representing a single sql statement
        if (sqlite3_prepare_v2(conn, sqlStatement.c_str(), -1, &statement, nullptr) == SQLITE_OK) {    // Compiling the sql statement into a bytecode saved in statement structure
            while (sqlite3_step(statement) == SQLITE_ROW) {    // Running the bytecode of the sql statement
                count = sqlite3_column_int(statement, 0);
            }
        } else {
            std::cerr << "Database Error, " << sqlite3_errmsg(conn) << std::endl;
            db_availability = false;
        }
        sqlite3_finalize(statement);    // Destroying the prepared statement object
        sqlite3_close(conn);    // Closing the db connection and destroying the handle
    } else {
        db_availability = false;
    }
    std::tuple<bool, bool> count_avail = {count, db_availability};
    return count_avail;
}

std::tuple<bool, bool> Database_Connection::get_paths(std::map<std::string, std::string> &paths, const std::string& username) {
    sqlite3* conn;  // Database handle defined by the sqlite3 structure
    unsigned char *paths_ch = nullptr;
    bool found = false;
    bool db_availability = true;
    if (sqlite3_open(db_name.data(), &conn) == SQLITE_OK) {
        std::string sqlStatement = std::string("SELECT paths FROM client WHERE username = '") + username + std::string("';");
        sqlite3_stmt *statement;    // Representing a single sql statement
        if (sqlite3_prepare_v2(conn, sqlStatement.c_str(), -1, &statement, nullptr) == SQLITE_OK) { // Compiling the sql statement into a bytecode saved in statement structure
            if (sqlite3_step(statement) == SQLITE_ROW) {  // Running the bytecode of the sql statement
                paths_ch = const_cast<unsigned char*>(sqlite3_column_text(statement, 0));   // Const cast in order to remove the const
            }
            if (paths_ch != nullptr) {
                std::stringstream paths_stream(reinterpret_cast<char*>(paths_ch));
                found = true;
                boost::property_tree::ptree pt;
                try {
                    boost::property_tree::read_json(paths_stream, pt);  // Re-creating json from data stream
                    for (auto pair : pt) {  // For every field the key-value pairs are saved in the paths map
                        std::stringstream hash_stream(pair.second.data());
                        std::string hash;
                        hash_stream >> hash;
                        paths[pair.first] = hash;
                    }
                } catch (const boost::property_tree::ptree_error &err) {
                    db_availability = false;
                }
            }
        } else {
            db_availability = false;
            std::cerr << "Database Connection Error, " << sqlite3_errmsg(conn) << std::endl;
        }
        sqlite3_finalize(statement);    // Destroying the prepared statement object
        sqlite3_close(conn);    // Closing the db connection and destroying the handle
    } else {
        db_availability = false;
    }
    return {found, db_availability};
}

bool Database_Connection::update_db_paths(std::map<std::string, std::string> &paths, const std::string& username) {
    std::cout << "Updating Database..." << std::endl;
    sqlite3* conn;  // Database handle defined by the sqlite3 structure
    bool db_availability = true;
    boost::property_tree::ptree pt;
    std::stringstream map_to_stream;
    try {
        if (!paths.empty()) {
            for (auto & path : paths)   // For every element of the map the key-value pairs are saved in a json file
                pt.add(path.first, path.second);
            boost::property_tree::write_json(map_to_stream, pt);    // Saving the json in a stream
        }
    } catch (const boost::property_tree::ptree_error &err) {
        std::cerr << "Error while writing json." << std::endl;
        db_availability = false;
    }
    if (sqlite3_open(db_name.data(), &conn) == SQLITE_OK && db_availability) {  // If the db can be opened and there are no other errors
        std::string sqlStatement;
        if (paths.empty()) {    // If the map is empty then set the field to NULL
            sqlStatement = std::string("UPDATE client SET paths = NULL WHERE username = '") + username + std::string("';");
        } else {    // Else set the field with the streamed map
            sqlStatement = std::string("UPDATE client SET paths = '") + map_to_stream.str() + std::string("' WHERE username = '") + username + std::string("';");
        }
        sqlite3_stmt *statement;   // Representing a single sql statement
        if (sqlite3_prepare_v2(conn, sqlStatement.c_str(), -1, &statement, nullptr) == SQLITE_OK) {    // Compiling the sql statement into a bytecode saved in statement structure
            sqlite3_step(statement);    // Running the bytecode of the sql statement
        } else {
            std::cerr << "Database Error, " << sqlite3_errmsg(conn) << std::endl;
            db_availability = false;
        }
        sqlite3_finalize(statement);    // Destroying the prepared statement object
        sqlite3_close(conn);        // Closing the db connection and destroying the handle
    } else {
        db_availability = false;
    }
    return db_availability;
}
