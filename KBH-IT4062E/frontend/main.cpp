#include <iostream>
#include "gui.h"
#include "unix_socket_client.h"

// TODO: Implement main entry point để khởi động ứng dụng frontend
int main() {
    GUI gui;  // Tạo đối tượng giao diện người chơi
    UNIXSocketClient client;  // Tạo đối tượng kết nối với server qua UNIX Socket

    // TODO: Thiết lập kết nối tới server, bắt đầu game và cập nhật giao diện
    client.connect_to_server();
    gui.start_game();

    return 0;
}
