#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <pqxx/pqxx>  // Thêm thư viện libpqxx

class Database {
public:
    Database(const std::string& db_conn_str);  // Cấu trúc kết nối PostgreSQL
    ~Database();

    // Lưu điểm người chơi vào cơ sở dữ liệu
    bool save_player_score(const std::string& player_name, int score);

    // Lấy leaderboard từ cơ sở dữ liệu
    std::vector<std::string> get_leaderboard();

private:
    pqxx::connection* conn_;  // Kết nối với PostgreSQL
    std::string db_conn_str_; // Cấu trúc chuỗi kết nối đến PostgreSQL
};

#endif
