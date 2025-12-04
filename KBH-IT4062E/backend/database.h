#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <pqxx/pqxx>  // Thêm thư viện libpqxx

class Database {
public:
    // Constructor with connection string
    Database(const std::string& db_conn_str);
    ~Database();

    // Static helper to build connection string with default credentials
    static std::string build_connection_string(
        const std::string& dbname = "kbh_db",
        const std::string& user = "postgres",
        const std::string& password = "postgres",
        const std::string& host = "localhost",
        int port = 5432
    ) {
        return "dbname=" + dbname + " user=" + user + " password=" + password +
               " host=" + host + " port=" + std::to_string(port);
    }

    // Lưu điểm người chơi vào cơ sở dữ liệu
    bool save_player_score(const std::string& player_name, int score);

    // Lấy leaderboard từ cơ sở dữ liệu
    std::vector<std::string> get_leaderboard();

private:
    pqxx::connection* conn_;  // Kết nối với PostgreSQL
    std::string db_conn_str_; // Cấu trúc chuỗi kết nối đến PostgreSQL
};

#endif
