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

std::vector<Database::LeaderboardEntry> Database::get_leaderboard_entries() {
    std::vector<LeaderboardEntry> leaderboard;

    try {
        pqxx::work txn(*conn_);
        pqxx::result r =
            txn.exec("SELECT player_name, score FROM leaderboard ORDER BY score DESC LIMIT 15;");

        int rank = 1;
        for (auto row : r) {
            LeaderboardEntry entry;
            entry.rank = rank++;
            entry.username = row[0].as<std::string>();
            entry.wpm = row[1].as<double>();
            leaderboard.push_back(entry);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error fetching leaderboard: " << e.what() << std::endl;
    }

    return leaderboard;
}

bool Database::create_account(const std::string& username, const std::string& password, std::string& err_msg) {
    try {
        pqxx::work txn(*conn_);
        pqxx::result r = txn.exec(
            "INSERT INTO app_user (username, password_hash, is_guest) VALUES (" +
            txn.quote(username) + ", crypt(" + txn.quote(password) + ", gen_salt('bf')), false) "
            "RETURNING user_id;");
        if (r.empty()) {
            err_msg = "CREATE_FAILED";
            return false;
        }
        int user_id = r[0][0].as<int>();
        txn.exec(
            "INSERT INTO user_profile (user_id, display_name) VALUES (" +
            std::to_string(user_id) + ", " + txn.quote(username) + ");");
        txn.commit();
        return true;
    }
    catch (const std::exception& e) {
        err_msg = e.what();
        std::cerr << "Error creating account: " << e.what() << std::endl;
        return false;
    }
}

bool Database::authenticate(const std::string& username, const std::string& password, int& user_id) {
    try {
        pqxx::work txn(*conn_);
        pqxx::result r = txn.exec(
            "SELECT user_id FROM app_user WHERE username = " + txn.quote(username) +
            " AND password_hash = crypt(" + txn.quote(password) + ", password_hash);");
        if (r.empty()) {
            return false;
        }
        user_id = r[0][0].as<int>();
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error authenticating: " << e.what() << std::endl;
        return false;
    }
}

bool Database::change_password(const std::string& username, const std::string& old_pwd, const std::string& new_pwd) {
    try {
        pqxx::work txn(*conn_);
        pqxx::result r = txn.exec(
            "UPDATE app_user SET password_hash = crypt(" + txn.quote(new_pwd) + ", gen_salt('bf')) "
            "WHERE username = " + txn.quote(username) +
            " AND password_hash = crypt(" + txn.quote(old_pwd) + ", password_hash);");
        txn.commit();
        return r.affected_rows() == 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Error changing password: " << e.what() << std::endl;
        return false;
    }
}

bool Database::save_result(const std::string& username, double wpm) {
    return save_player_score(username, static_cast<int>(wpm));
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
