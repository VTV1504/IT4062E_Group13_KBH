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
// -------------------------------------------
// Authentication: kbh_authenticate
// -------------------------------------------
std::pair<int64_t, std::string> Database::authenticate(const std::string& username, const std::string& password) {
    try {
        pqxx::work txn(*conn_);
        
        // Call the kbh_authenticate function
        pqxx::result r = txn.exec_params(
            "SELECT * FROM kbh_authenticate($1, $2)",
            username, password
        );
        
        if (r.empty()) {
            // Authentication failed
            return {-1, ""};
        }
        
        int64_t user_id = r[0][0].as<int64_t>();
        std::string returned_username = r[0][1].as<std::string>();
        
        return {user_id, returned_username};
    }
    catch (const std::exception& e) {
        std::cerr << "DB authenticate error: " << e.what() << std::endl;
        return {-1, ""};
    }
}

// -------------------------------------------
// Create user: kbh_create_user
// -------------------------------------------
int64_t Database::create_user(const std::string& username, const std::string& password) {
    try {
        pqxx::work txn(*conn_);
        
        // Call the kbh_create_user function
        pqxx::result r = txn.exec_params(
            "SELECT kbh_create_user($1, $2)",
            username, password
        );
        
        if (r.empty()) {
            return -1;
        }
        
        int64_t user_id = r[0][0].as<int64_t>();
        txn.commit();
        
        return user_id;
    }
    catch (const std::exception& e) {
        std::cerr << "DB create_user error: " << e.what() << std::endl;
        return -1;
    }
}

// -------------------------------------------
// Change password: kbh_change_password
// -------------------------------------------
bool Database::change_password(const std::string& username, const std::string& old_password, const std::string& new_password) {
    try {
        pqxx::work txn(*conn_);
        
        // Call the kbh_change_password function
        pqxx::result r = txn.exec_params(
            "SELECT kbh_change_password($1, $2, $3)",
            username, old_password, new_password
        );
        
        if (r.empty()) {
            return false;
        }
        
        bool success = r[0][0].as<bool>();
        txn.commit();
        
        return success;
    }
    catch (const std::exception& e) {
        std::cerr << "DB change_password error: " << e.what() << std::endl;
        return false;
    }
}

// -------------------------------------------
// Get paragraph_id by body text
// -------------------------------------------
int64_t Database::get_paragraph_id(const std::string& paragraph_body) {
    try {
        pqxx::work txn(*conn_);
        
        pqxx::result r = txn.exec_params(
            "SELECT paragraph_id FROM paragraph WHERE body = $1 LIMIT 1",
            paragraph_body
        );
        
        if (r.empty()) {
            return -1;  // Paragraph not found
        }
        
        return r[0][0].as<int64_t>();
    }
    catch (const std::exception& e) {
        std::cerr << "DB get_paragraph_id error: " << e.what() << std::endl;
        return -1;
    }
}

// -------------------------------------------
// Save training result
// -------------------------------------------
bool Database::save_training_result(int64_t user_id, const std::string& paragraph_body,
                                     double wpm, double accuracy,
                                     int duration_ms, int words_committed) {
    try {
        pqxx::work txn(*conn_);
        
        // Get paragraph_id first
        int64_t paragraph_id = get_paragraph_id(paragraph_body);
        
        // Insert game result
        if (paragraph_id == -1) {
            // paragraph not found, insert with NULL paragraph_id
            txn.exec_params(
                "INSERT INTO game_result (user_id, paragraph_id, wpm, accuracy, duration_ms, words_committed) "
                "VALUES ($1, NULL, $2, $3, $4, $5)",
                user_id, 
                wpm, 
                accuracy, 
                duration_ms, 
                words_committed
            );
        } else {
            txn.exec_params(
                "INSERT INTO game_result (user_id, paragraph_id, wpm, accuracy, duration_ms, words_committed) "
                "VALUES ($1, $2, $3, $4, $5, $6)",
                user_id, 
                paragraph_id,
                wpm, 
                accuracy, 
                duration_ms, 
                words_committed
            );
        }
        
        txn.commit();
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "DB save_training_result error: " << e.what() << std::endl;
        return false;
    }
}

// -------------------------------------------
// Get top players in the last week
// -------------------------------------------
std::vector<LeaderboardEntry> Database::get_top_players(int limit) {
    std::vector<LeaderboardEntry> entries;
    
    try {
        pqxx::work txn(*conn_);
        
        // Get top players from last 7 days, ordered by max WPM
        pqxx::result r = txn.exec_params(
            "WITH ranked_results AS ( "
            "  SELECT "
            "    u.username, "
            "    MAX(gr.wpm) as best_wpm, "
            "    RANK() OVER (ORDER BY MAX(gr.wpm) DESC) as rank "
            "  FROM game_result gr "
            "  JOIN app_user u ON u.user_id = gr.user_id "
            "  WHERE gr.created_at >= NOW() - INTERVAL '7 days' "
            "    AND gr.user_id IS NOT NULL "
            "  GROUP BY u.username "
            ") "
            "SELECT rank, username, best_wpm "
            "FROM ranked_results "
            "ORDER BY rank "
            "LIMIT $1",
            limit
        );
        
        for (const auto& row : r) {
            LeaderboardEntry entry;
            entry.rank = row["rank"].as<int>();
            entry.username = row["username"].as<std::string>();
            entry.wpm = row["best_wpm"].as<double>();
            entries.push_back(entry);
        }
        
        txn.commit();
    }
    catch (const std::exception& e) {
        std::cerr << "DB get_top_players error: " << e.what() << std::endl;
    }
    
    return entries;
}

// -------------------------------------------
// Get user rank in the last week
// -------------------------------------------
LeaderboardEntry Database::get_user_rank(int64_t user_id) {
    LeaderboardEntry entry{0, "", 0.0};  // rank=0 means not found
    
    try {
        pqxx::work txn(*conn_);
        
        // Get user's rank based on best WPM in last 7 days
        pqxx::result r = txn.exec_params(
            "WITH ranked_results AS ( "
            "  SELECT "
            "    gr.user_id, "
            "    u.username, "
            "    MAX(gr.wpm) as best_wpm, "
            "    RANK() OVER (ORDER BY MAX(gr.wpm) DESC) as rank "
            "  FROM game_result gr "
            "  JOIN app_user u ON u.user_id = gr.user_id "
            "  WHERE gr.created_at >= NOW() - INTERVAL '7 days' "
            "    AND gr.user_id IS NOT NULL "
            "  GROUP BY gr.user_id, u.username "
            ") "
            "SELECT rank, username, best_wpm "
            "FROM ranked_results "
            "WHERE user_id = $1",
            user_id
        );
        
        if (!r.empty()) {
            entry.rank = r[0]["rank"].as<int>();
            entry.username = r[0]["username"].as<std::string>();
            entry.wpm = r[0]["best_wpm"].as<double>();
        }
        
        txn.commit();
    }
    catch (const std::exception& e) {
        std::cerr << "DB get_user_rank error: " << e.what() << std::endl;
    }
    
    return entry;
}