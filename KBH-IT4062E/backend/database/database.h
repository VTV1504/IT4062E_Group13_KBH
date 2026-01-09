#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <pqxx/pqxx>
#include <jsoncpp/json/json.h>

class Database {
public:
    // Constructor
    Database(const std::string& db_conn_str);

    // Destructor
    ~Database();

    void init_schema();

    bool create_user(const std::string& username,
                     const std::string& password_hash,
                     const std::string& salt);
    bool verify_user(const std::string& username,
                     const std::string& password_hash) const;
    bool update_password(const std::string& username,
                         const std::string& new_password_hash,
                         const std::string& new_salt);
    bool user_exists(const std::string& username) const;

    std::string get_user_salt(const std::string& username) const;

    bool save_self_training_result(const std::string& username,
                                   double wpm,
                                   double accuracy);
    Json::Value get_self_training_leaderboard(const std::string& username) const;

    bool save_survival_result(const std::string& username,
                              int points,
                              int rooms_survived);
    Json::Value get_survival_leaderboard(const std::string& username) const;

    std::string get_paragraph_by_word_count(int word_count,
                                            const std::string& language = "en");
    std::string get_random_paragraph(const std::string& language = "en");

private:
    pqxx::connection* conn_;
    std::string db_conn_str_;
};

#endif
