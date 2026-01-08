#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <pqxx/pqxx>

class Database {
public:
    struct LeaderboardEntry {
        int rank = 0;
        std::string username;
        double wpm = 0.0;
    };

    // Constructor
    Database(const std::string& db_conn_str);

    // Destructor
    ~Database();

    // Save score (future feature)
    bool save_player_score(const std::string& player_name, int score);

    // Get leaderboard (future feature)
    std::vector<std::string> get_leaderboard();
    std::vector<LeaderboardEntry> get_leaderboard_entries();

    bool create_account(const std::string& username, const std::string& password, std::string& err_msg);
    bool authenticate(const std::string& username, const std::string& password, int& user_id);
    bool change_password(const std::string& username, const std::string& old_pwd, const std::string& new_pwd);
    bool save_result(const std::string& username, double wpm);

    // 🔥 NEW: get random paragraph for Arena Mode
    std::string get_random_paragraph(const std::string& language = "en");

private:
    pqxx::connection* conn_;
    std::string db_conn_str_;
};

#endif
