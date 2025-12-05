#include <iostream>
#include "server.h"
#include "config.h"
#include "database.h"

int main() {
    std::cout << "=== Keyboard Heroes Arena Server ===\n";

    Config config("config/server_config.json");
    std::string server_ip   = config.get_server_ip();
    int         server_port = config.get_server_port();
    std::string db_conn_str = config.get_config_value("db_conn_str");

    std::cout << "[CONFIG] IP: " << server_ip
              << "  PORT: " << server_port << "\n";

    Database db(db_conn_str);

    Server server(server_ip, server_port, &db);
    std::cout << "[SERVER] Starting...\n";
    server.start();

    return 0;
}
