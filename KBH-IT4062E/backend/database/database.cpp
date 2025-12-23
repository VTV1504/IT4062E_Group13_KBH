#include "database.h"
#include <pqxx/pqxx>
#include <iostream>
#include <string>

// -------------------------------------------
// Constructor
// -------------------------------------------
Database::Database(const std::string& db_conn_str)
    : conn_(nullptr), db_conn_str_(db_conn_str)
{
    try {
        conn_ = new pqxx::connection(db_conn_str_);

        if (!conn_->is_open()) {
            std::cerr << "Failed to open database." << std::endl;
            throw std::runtime_error("Database connection failed.");
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Database connection error: " << e.what() << std::endl;
        throw;
    }
}

// -------------------------------------------
// Destructor
// -------------------------------------------
Database::~Database() {
    if (conn_ != nullptr) {
        delete conn_;
        conn_ = nullptr;
    }
}

// -------------------------------------------
// Save player score (optional)
// -------------------------------------------
bool Database::save_player_score(const std::string& player_name, int score) {
    try {
        pqxx::work txn(*conn_);

        std::string query =
            "INSERT INTO leaderboard (player_name, score) VALUES (" +
            txn.quote(player_name) + ", " + std::to_string(score) + ");";

        txn.exec(query);
        txn.commit();
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error inserting score: " << e.what() << std::endl;
        return false;
    }
}

// -------------------------------------------
// Get leaderboard (optional)
// -------------------------------------------
std::vector<std::string> Database::get_leaderboard() {
    std::vector<std::string> leaderboard;

    try {
        pqxx::work txn(*conn_);
        pqxx::result r =
            txn.exec("SELECT player_name, score FROM leaderboard ORDER BY score DESC LIMIT 50;");

        for (auto row : r) {
            leaderboard.push_back(
                row[0].as<std::string>() + ": " + std::to_string(row[1].as<int>())
            );
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error fetching leaderboard: " << e.what() << std::endl;
    }

    return leaderboard;
}

// -------------------------------------------
// NEW: Get a random paragraph from DB
// -------------------------------------------
std::string Database::get_random_paragraph(const std::string& language) {
    try {
        pqxx::work txn(*conn_);

        pqxx::result r =
            txn.exec("SELECT body FROM paragraph "
                     "WHERE language = " + txn.quote(language) + " "
                     "AND is_active = TRUE "
                     "ORDER BY random() LIMIT 1;");

        if (r.empty()) {
            return "No paragraph available.";
        }

        return r[0][0].as<std::string>();
    }
    catch (const std::exception& e) {
        std::cerr << "DB paragraph error: " << e.what() << std::endl;
        return "Error loading paragraph.";
    }
}
