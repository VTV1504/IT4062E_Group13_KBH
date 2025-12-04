#include "database.h"
#include <pqxx/pqxx>  // Thư viện libpqxx
#include <iostream>

// Constructor kết nối tới PostgreSQL với chuỗi kết nối
Database::Database(const std::string& db_conn_str) : db_conn_str_(db_conn_str) {
    try {
        conn_ = new pqxx::connection(db_conn_str_);
        if (!conn_->is_open()) {
            std::cerr << "Can't open database" << std::endl;  // Thông báo lỗi khi không thể kết nối
            throw std::runtime_error("Database connection failed.");
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        throw;
    }
}

// Destructor (Không cần gọi conn_->close() nữa vì sẽ tự động đóng khi đối tượng bị hủy)
Database::~Database() {
    // conn_ sẽ tự động được hủy và đóng kết nối khi đối tượng Database bị hủy
    delete conn_;  // Hủy đối tượng conn_ để giải phóng bộ nhớ
}

// Lưu điểm người chơi vào PostgreSQL
bool Database::save_player_score(const std::string& player_name, int score) {
    try {
        pqxx::work txn(*conn_);
        std::string query = "INSERT INTO leaderboard (player_name, score) VALUES (" +
                            txn.quote(player_name) + ", " + std::to_string(score) + ");";
        txn.exec(query);
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error inserting score: " << e.what() << std::endl;  // In lỗi nếu không thể lưu điểm
        return false;
    }
}

// Lấy leaderboard từ PostgreSQL
std::vector<std::string> Database::get_leaderboard() {
    std::vector<std::string> leaderboard;
    try {
        pqxx::work txn(*conn_);
        pqxx::result result = txn.exec("SELECT player_name, score FROM leaderboard ORDER BY score DESC LIMIT 50;");
        
        for (const auto& row : result) {
            std::string player_name = row[0].as<std::string>();
            int score = row[1].as<int>();
            leaderboard.push_back(player_name + ": " + std::to_string(score));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error fetching leaderboard: " << e.what() << std::endl;  // In lỗi nếu không thể lấy leaderboard
    }
    return leaderboard;
}
