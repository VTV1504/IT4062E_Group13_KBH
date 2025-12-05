#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <jsoncpp/json/json.h>

class Config {
public:
    Config(const std::string& config_file);  // Khởi tạo và đọc cấu hình từ file
    ~Config();

    // Đọc giá trị cấu hình bất kỳ dưới dạng string
    std::string get_config_value(const std::string& key);

    // Lấy địa chỉ server (legacy: có thể là "localhost" hoặc "127.0.0.1")
    std::string get_server_address();

    // Lấy đường dẫn socket UNIX (nếu vẫn dùng UNIX domain socket ở chỗ khác)
    std::string get_socket_path();

    // ===== Thêm cho TCP server giữa nhiều máy =====

    // Lấy IP server cho TCP (ưu tiên "server_ip", fallback "server_address")
    std::string get_server_ip();

    // Lấy port server cho TCP (key "server_port", mặc định 5000 nếu thiếu)
    int get_server_port();

private:
    void load_config(const std::string& config_file);

    // Dữ liệu cấu hình (JsonCpp)
    Json::Value config_data_;
};

#endif
