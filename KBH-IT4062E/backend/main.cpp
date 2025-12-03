#include <iostream>
#include "server.h"
#include "game_manager.h"
#include "config.h"

int main() {
    // TODO: Đọc cấu hình từ file config (ví dụ: cấu trúc kết nối database)
    Config config("config/server_config.json");

    // TODO: Lấy thông tin cấu hình cần thiết
    std::string db_conn_str = config.get_config_value("db_conn_str");
    
    // Khởi tạo server với đường dẫn UNIX socket
    Server server("/tmp/keyboard_heroes_socket");
    
    // Khởi tạo GameManager
    GameManager game_manager;

    // Khởi động server (lắng nghe kết nối)
    server.start();

    // TODO: Sau khi kết nối, GameManager sẽ xử lý các chế độ chơi
    game_manager.start_game("Arena");

    return 0;
}
