#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <jsoncpp/json/json.h>  // Nếu bạn dùng JsonCpp để đọc cấu hình JSON

class Config {
public:
    Config(const std::string& config_file);  // Constructor: Đọc cấu hình từ file
    ~Config();  // Destructor

    // TODO: Hàm để lấy giá trị cấu hình từ file JSON
    std::string get_config_value(const std::string& key);

    // TODO: Hàm lấy thông tin về server address (ví dụ: địa chỉ server hoặc UNIX socket path)
    std::string get_server_address();

    // TODO: Hàm lấy đường dẫn UNIX socket
    std::string get_socket_path();

private:
    Json::Value config_data_;  // Lưu trữ dữ liệu cấu hình từ JSON
    void load_config(const std::string& config_file);  // Hàm đọc cấu hình từ file JSON
};

#endif
