#ifndef SERVER_H
#define SERVER_H

#include <string>

class Server {
public:
    Server(const std::string& socket_path);  // TODO: Khởi tạo với đường dẫn UNIX Socket
    ~Server();  // TODO: Destructor để đóng socket

    // TODO: Hàm bắt đầu server, lắng nghe kết nối
    void start();

    // TODO: Hàm xử lý kết nối từ client
    void handle_client(int client_fd);

private:
    std::string socket_path_;  // Đường dẫn UNIX socket
    int server_fd_;  // Mã mô tả socket server
};

#endif
