#include "Database_Connection.h"

Database_Connection::Database_Connection(): db_name("../Clients.sqlite"){};


std::tuple<bool, bool> Database_Connection::check_database(const std::string& temp_username, const std::string& password) {
    std::cout << "Checking Database..." << std::endl;
    int count = 0;
    bool db_availability = true;
    if (sqlite3_open(db_name.data(), &conn) == SQLITE_OK) {
        std::string sqlStatement = std::string("SELECT COUNT(*) FROM Client WHERE username = '") + temp_username + std::string("' AND password = '") + password + std::string("';");
        sqlite3_stmt *statement;
        int res = sqlite3_prepare_v2(conn, sqlStatement.c_str(), -1, &statement, nullptr);
        if (res == SQLITE_OK) {
            while( sqlite3_step(statement) == SQLITE_ROW ) {
                count = sqlite3_column_int(statement, 0);
            }
        } else {
            std::cerr << "Database Error, " << sqlite3_errmsg(conn) << std::endl;
            db_availability = false;
        }
        sqlite3_finalize(statement);
        sqlite3_close(conn);
    } else {
        db_availability = false;
    }
    std::tuple<bool, bool> count_avail = {count, db_availability};
    return count_avail;
}


bool Database_Connection::update_db_paths(std::map<std::string, std::size_t> &paths, std::string username) {
    std::cout << "Updating Database..." << std::endl;
    bool db_availability = true;
    boost::property_tree::ptree pt;
    std::stringstream map_to_stream;
    try {
        for (auto & path : paths)
            pt.add(path.first, path.second);
        boost::property_tree::write_json(map_to_stream, pt);
    } catch (const boost::property_tree::ptree_error &err) {
        std::cerr << "Error while writing json." << std::endl;
        throw;
    }
    if (sqlite3_open(db_name.data(), &conn) == SQLITE_OK) {
        std::string sqlStatement = std::string("UPDATE client SET paths = '") + map_to_stream.str() + std::string("' WHERE username = '") + username + std::string("';");
        sqlite3_stmt *statement;
        int res = sqlite3_prepare_v2(conn, sqlStatement.c_str(), -1, &statement, nullptr);
        if (res == SQLITE_OK) {
            sqlite3_step(statement);
        } else {
            std::cerr << "Database Error, " << sqlite3_errmsg(conn) << std::endl;
            db_availability = false;
        }
        sqlite3_finalize(statement);
        sqlite3_close(conn);
    } else {
        db_availability = false;
    }
    return db_availability;
}


std::tuple<bool, bool> Database_Connection::get_paths(std::map<std::string, std::size_t> &paths, std::string username) {
    unsigned char *paths_ch = nullptr;
    bool found = false;
    bool db_availability = true;
    if (sqlite3_open(db_name.data(), &conn) == SQLITE_OK) {
        std::string sqlStatement = std::string("SELECT paths FROM client WHERE username = '") + username + std::string("';");
        sqlite3_stmt *statement;
        if (sqlite3_prepare_v2(conn, sqlStatement.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
            if ( sqlite3_step(statement) == SQLITE_ROW ) {
                paths_ch = const_cast<unsigned char*>(sqlite3_column_text(statement, 0));
            }
            if (paths_ch != nullptr) {
                std::stringstream paths_stream(reinterpret_cast<char*>(paths_ch));
                found = true;
                boost::property_tree::ptree pt;
                try {
                    boost::property_tree::read_json(paths_stream, pt);
                    for (auto pair : pt) {
                        std::stringstream hash_stream(pair.second.data());
                        size_t hash;
                        hash_stream >> hash;
                        paths[pair.first] = hash;
                    }
                } catch (const boost::property_tree::ptree_error &err) {
                    throw;
                }
            }
        } else {
            db_availability = false;
            std::cerr << "Database Connection Error, " << sqlite3_errmsg(conn) << std::endl;
        }
        sqlite3_finalize(statement);
        sqlite3_close(conn);
    } else {
        db_availability = false;
    }
    std::tuple<bool, bool> found_avail = {found, db_availability};
    return found_avail;
}
