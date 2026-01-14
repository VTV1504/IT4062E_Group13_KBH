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

    // Get random paragraph for Arena Mode
    std::string get_random_paragraph(const std::string& language = "en");
    
    // Get paragraph_id by body text
    int64_t get_paragraph_id(const std::string& paragraph_body);
    
    // Save training result
    bool save_training_result(int64_t user_id, const std::string& paragraph_body, 
                              double wpm, double accuracy, 
                              int duration_ms, int words_committed);
    
    // Authentication methods
    std::pair<int64_t, std::string> authenticate(const std::string& username, const std::string& password);
    int64_t create_user(const std::string& username, const std::string& password);
    bool change_password(const std::string& username, const std::string& old_password, const std::string& new_password);

private:
    pqxx::connection* conn_;
    std::string db_conn_str_;
};

#endif
