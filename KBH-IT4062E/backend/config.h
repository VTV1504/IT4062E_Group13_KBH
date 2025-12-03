#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <jsoncpp/json/json.h>

class Config {
public:
    Config(const std::string& config_file);  // Khởi tạo và đọc cấu hình từ file
    ~Config();

    // TODO: Phương thức đọc giá trị cấu hình
    std::string get_config_value(const std::string& key);
    // Lấy địa chỉ server (ví dụ PostgreSQL hoặc địa chỉ UNIX socket)
    std::string get_server_address();

    // Lấy đường dẫn socket UNIX
    std::string get_socket_path();

private:
    // TODO: Lưu trữ dữ liệu cấu hình (có thể dùng JsonCpp hoặc một thư viện JSON khác)
    void load_config(const std::string& config_file);

    // Dữ liệu cấu hình (JsonCpp)
    Json::Value config_data_;
};

#endif
