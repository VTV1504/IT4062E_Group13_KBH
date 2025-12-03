#include "config.h"
#include <iostream>
#include <fstream>
#include <jsoncpp/json/json.h>  // Nếu bạn sử dụng JsonCpp để đọc cấu hình JSON

// Constructor: Đọc cấu hình từ file
Config::Config(const std::string& config_file) {
    load_config(config_file);
}

// Destructor
Config::~Config() {
    // TODO: Giải phóng tài nguyên nếu cần
}

// Hàm đọc và tải cấu hình từ file JSON
void Config::load_config(const std::string& config_file) {
    std::ifstream config_stream(config_file, std::ifstream::binary);
    if (config_stream.is_open()) {
        config_stream >> config_data_;  // Đọc toàn bộ cấu hình từ file JSON
    } else {
        std::cerr << "Failed to load config file: " << config_file << std::endl;
    }
}

// Hàm lấy giá trị cấu hình từ JSON
std::string Config::get_config_value(const std::string& key) {
    return config_data_[key].asString();  // Trả về giá trị của key tương ứng từ JSON
}

// Hàm lấy chuỗi kết nối server (ví dụ PostgreSQL hoặc địa chỉ UNIX socket)
std::string Config::get_server_address() {
    return config_data_["server_address"].asString();
}

// Hàm lấy thông tin về cổng UNIX socket
std::string Config::get_socket_path() {
    return config_data_["socket_path"].asString();
}
