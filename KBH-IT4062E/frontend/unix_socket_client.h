#ifndef UNIX_SOCKET_CLIENT_H
#define UNIX_SOCKET_CLIENT_H

#include <string>

class UNIXSocketClient {
public:
    UNIXSocketClient();  // Constructor
    void connect_to_server();  // Kết nối tới server
    void send_message(const std::string& message);  // Gửi thông điệp
    std::string receive_message();  // Nhận thông điệp từ server

private:
    int client_fd_;  // Mã mô tả socket client
    const std::string SOCKET_PATH = "/tmp/keyboard_heroes_socket";  // Đường dẫn UNIX Socket
};

#endif
