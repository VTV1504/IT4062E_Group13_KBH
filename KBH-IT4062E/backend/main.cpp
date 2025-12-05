#include <iostream>
#include "server.h"
#include "game_manager.h"
#include "config.h"
#include "database.h"

int main() {
    std::cout << "=== Keyboard Heroes Arena Server ===\n";

    // Load config
    Config config("config/server_config.json");
    std::string server_ip   = config.get_server_ip();
    int         server_port = config.get_server_port();
    std::string db_conn_str = config.get_config_value("db_conn_str");

    std::cout << "[CONFIG] IP: " << server_ip
              << "  PORT: " << server_port << "\n";

    // Khởi tạo database (PostgreSQL)
    Database db(db_conn_str);

    // GameManager + gán Database
    GameManager game_manager;
    game_manager.set_database(&db);

    // Khởi tạo Arena mode (sẽ lấy text từ DB)
    game_manager.start_game("Arena");

    // Tạo TCP server, truyền con trỏ GameManager
    Server server(server_ip, server_port, &game_manager);

    std::cout << "[SERVER] Starting...\n";
    server.start();

    return 0;
}
