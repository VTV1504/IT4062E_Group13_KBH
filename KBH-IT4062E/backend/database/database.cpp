#include "database.h"
#include <pqxx/pqxx>
#include <iostream>
#include <random>

Database::Database(const std::string& db_conn_str)
    : conn_(nullptr), db_conn_str_(db_conn_str)
{
    try {
        conn_ = new pqxx::connection(db_conn_str_);

        if (!conn_->is_open()) {
            std::cerr << "Failed to open database." << std::endl;
            throw std::runtime_error("Database connection failed.");
        }

        init_schema();
    }
    catch (const std::exception& e) {
        std::cerr << "Database connection error: " << e.what() << std::endl;
        throw;
    }
}

Database::~Database() {
    if (conn_ != nullptr) {
        delete conn_;
        conn_ = nullptr;
    }
}

void Database::init_schema() {
    try {
        pqxx::work txn(*conn_);
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS users (
                id SERIAL PRIMARY KEY,
                username VARCHAR(32) UNIQUE NOT NULL,
                password_hash TEXT NOT NULL,
                salt TEXT NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );

            CREATE TABLE IF NOT EXISTS paragraphs (
                id SERIAL PRIMARY KEY,
                body TEXT NOT NULL,
                word_count INTEGER NOT NULL,
                language VARCHAR(8) DEFAULT 'en',
                is_active BOOLEAN DEFAULT TRUE
            );

            CREATE INDEX IF NOT EXISTS idx_paragraphs_word_count
                ON paragraphs(word_count);
            CREATE INDEX IF NOT EXISTS idx_paragraphs_language
                ON paragraphs(language);

            CREATE TABLE IF NOT EXISTS self_training_results (
                id SERIAL PRIMARY KEY,
                user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
                wpm REAL NOT NULL,
                accuracy REAL NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );

            CREATE INDEX IF NOT EXISTS idx_self_training_user
                ON self_training_results(user_id);

            CREATE TABLE IF NOT EXISTS survival_results (
                id SERIAL PRIMARY KEY,
                user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
                points INTEGER NOT NULL,
                rooms_survived INTEGER NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );

            CREATE INDEX IF NOT EXISTS idx_survival_user
                ON survival_results(user_id);
        )");
        txn.commit();
    } catch (const std::exception& e) {
        std::cerr << "Schema init error: " << e.what() << std::endl;
        throw;
    }
}

bool Database::create_user(const std::string& username,
                           const std::string& password_hash,
                           const std::string& salt) {
    try {
        pqxx::work txn(*conn_);
        txn.exec_params(
            "INSERT INTO users (username, password_hash, salt) VALUES ($1, $2, $3);",
            username, password_hash, salt);
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Create user error: " << e.what() << std::endl;
        return false;
    }
}

bool Database::verify_user(const std::string& username,
                           const std::string& password_hash) const {
    try {
        pqxx::work txn(*conn_);
        pqxx::result r = txn.exec_params(
            "SELECT 1 FROM users WHERE username = $1 AND password_hash = $2;",
            username, password_hash);
        return !r.empty();
    } catch (const std::exception& e) {
        std::cerr << "Verify user error: " << e.what() << std::endl;
        return false;
    }
}

bool Database::update_password(const std::string& username,
                               const std::string& new_password_hash,
                               const std::string& new_salt) {
    try {
        pqxx::work txn(*conn_);
        pqxx::result r = txn.exec_params(
            "UPDATE users SET password_hash = $2, salt = $3 WHERE username = $1;",
            username, new_password_hash, new_salt);
        txn.commit();
        return r.affected_rows() == 1;
    } catch (const std::exception& e) {
        std::cerr << "Update password error: " << e.what() << std::endl;
        return false;
    }
}

bool Database::user_exists(const std::string& username) const {
    try {
        pqxx::work txn(*conn_);
        pqxx::result r = txn.exec_params(
            "SELECT 1 FROM users WHERE username = $1;",
            username);
        return !r.empty();
    } catch (const std::exception& e) {
        std::cerr << "User exists error: " << e.what() << std::endl;
        return false;
    }
}

std::string Database::get_user_salt(const std::string& username) const {
    try {
        pqxx::work txn(*conn_);
        pqxx::result r = txn.exec_params(
            "SELECT salt FROM users WHERE username = $1;",
            username);
        if (r.empty()) {
            return {};
        }
        return r[0][0].as<std::string>();
    } catch (const std::exception& e) {
        std::cerr << "Get salt error: " << e.what() << std::endl;
        return {};
    }
}

bool Database::save_self_training_result(const std::string& username,
                                         double wpm,
                                         double accuracy) {
    try {
        pqxx::work txn(*conn_);
        pqxx::result r = txn.exec_params(
            "SELECT id FROM users WHERE username = $1;",
            username);
        if (r.empty()) {
            return false;
        }
        int user_id = r[0][0].as<int>();
        txn.exec_params(
            "INSERT INTO self_training_results (user_id, wpm, accuracy) VALUES ($1, $2, $3);",
            user_id, wpm, accuracy);
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Save self-training result error: " << e.what() << std::endl;
        return false;
    }
}

Json::Value Database::get_self_training_leaderboard(const std::string& username) const {
    Json::Value result;
    result["type"] = "SELF_TRAINING_LEADERBOARD";
    Json::Value entries(Json::arrayValue);

    try {
        pqxx::work txn(*conn_);
        pqxx::result r = txn.exec(
            "SELECT u.username, AVG(r.wpm) AS avg_wpm, AVG(r.accuracy) AS avg_accuracy "
            "FROM self_training_results r "
            "JOIN users u ON u.id = r.user_id "
            "GROUP BY u.username "
            "ORDER BY avg_wpm DESC, avg_accuracy DESC "
            "LIMIT 15;");

        for (const auto& row : r) {
            Json::Value entry;
            entry["username"] = row[0].as<std::string>();
            entry["avg_wpm"] = row[1].as<double>();
            entry["avg_accuracy"] = row[2].as<double>();
            entries.append(entry);
        }

        result["leaderboard"] = entries;

        pqxx::result rank_r = txn.exec_params(
            "SELECT rank FROM ("
            "  SELECT u.username, "
            "         RANK() OVER (ORDER BY AVG(r.wpm) DESC, AVG(r.accuracy) DESC) AS rank "
            "  FROM self_training_results r "
            "  JOIN users u ON u.id = r.user_id "
            "  GROUP BY u.username"
            ") t WHERE username = $1;",
            username);
        if (!rank_r.empty()) {
            result["player_rank"] = rank_r[0][0].as<int>();
        }
    } catch (const std::exception& e) {
        std::cerr << "Leaderboard error: " << e.what() << std::endl;
    }

    return result;
}

bool Database::save_survival_result(const std::string& username,
                                    int points,
                                    int rooms_survived) {
    try {
        pqxx::work txn(*conn_);
        pqxx::result r = txn.exec_params(
            "SELECT id FROM users WHERE username = $1;",
            username);
        if (r.empty()) {
            return false;
        }
        int user_id = r[0][0].as<int>();
        txn.exec_params(
            "INSERT INTO survival_results (user_id, points, rooms_survived) VALUES ($1, $2, $3);",
            user_id, points, rooms_survived);
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Save survival result error: " << e.what() << std::endl;
        return false;
    }
}

Json::Value Database::get_survival_leaderboard(const std::string& username) const {
    Json::Value result;
    result["type"] = "SURVIVAL_LEADERBOARD";
    Json::Value entries(Json::arrayValue);

    try {
        pqxx::work txn(*conn_);
        pqxx::result r = txn.exec(
            "SELECT u.username, SUM(r.points) AS total_points, SUM(r.rooms_survived) AS total_rooms "
            "FROM survival_results r "
            "JOIN users u ON u.id = r.user_id "
            "GROUP BY u.username "
            "ORDER BY total_points DESC, total_rooms DESC "
            "LIMIT 15;");

        for (const auto& row : r) {
            Json::Value entry;
            entry["username"] = row[0].as<std::string>();
            entry["total_points"] = row[1].as<int>();
            entry["total_rooms"] = row[2].as<int>();
            entries.append(entry);
        }

        result["leaderboard"] = entries;

        pqxx::result rank_r = txn.exec_params(
            "SELECT rank FROM ("
            "  SELECT u.username, "
            "         RANK() OVER (ORDER BY SUM(r.points) DESC, SUM(r.rooms_survived) DESC) AS rank "
            "  FROM survival_results r "
            "  JOIN users u ON u.id = r.user_id "
            "  GROUP BY u.username"
            ") t WHERE username = $1;",
            username);
        if (!rank_r.empty()) {
            result["player_rank"] = rank_r[0][0].as<int>();
        }
    } catch (const std::exception& e) {
        std::cerr << "Survival leaderboard error: " << e.what() << std::endl;
    }

    return result;
}

std::string Database::get_paragraph_by_word_count(int word_count,
                                                  const std::string& language) {
    try {
        pqxx::work txn(*conn_);
        pqxx::result r = txn.exec_params(
            "SELECT body FROM paragraphs "
            "WHERE language = $1 AND is_active = TRUE "
            "AND word_count BETWEEN $2 AND $3 "
            "ORDER BY random() LIMIT 1;",
            language, word_count - 5, word_count + 5);

        if (r.empty()) {
            return get_random_paragraph(language);
        }

        return r[0][0].as<std::string>();
    } catch (const std::exception& e) {
        std::cerr << "DB paragraph error: " << e.what() << std::endl;
        return "Error loading paragraph.";
    }
}

std::string Database::get_random_paragraph(const std::string& language) {
    try {
        pqxx::work txn(*conn_);

        pqxx::result r =
            txn.exec_params("SELECT body FROM paragraphs "
                            "WHERE language = $1 "
                            "AND is_active = TRUE "
                            "ORDER BY random() LIMIT 1;",
                            language);

        if (r.empty()) {
            return "No paragraph available.";
        }

        return r[0][0].as<std::string>();
    } catch (const std::exception& e) {
        std::cerr << "DB paragraph error: " << e.what() << std::endl;
        return "Error loading paragraph.";
    }
}
