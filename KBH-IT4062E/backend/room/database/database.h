#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <pqxx/pqxx>

class Database {
public:
    // Constructor
    Database(const std::string& db_conn_str);

    // Destructor
    ~Database();

    // Save score (future feature)
    bool save_player_score(const std::string& player_name, int score);

    // Get leaderboard (future feature)
    std::vector<std::string> get_leaderboard();

    // 🔥 NEW: get random paragraph for Arena Mode
    std::string get_random_paragraph(const std::string& language = "en");

private:
    pqxx::connection* conn_;
    std::string db_conn_str_;
};

#endif
