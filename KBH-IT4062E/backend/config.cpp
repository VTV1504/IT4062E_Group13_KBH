#include "config.h"
#include <fstream>
#include <iostream>
#include <jsoncpp/json/json.h>  // Thư viện JsonCpp để xử lý JSON

// Constructor: Đọc cấu hình từ file
Config::Config(const std::string& config_file) {
    load_config(config_file);
}

// Destructor
Config::~Config() {
    // TODO: Giải phóng tài nguyên nếu cần
}

// Hàm đọc cấu hình từ file JSON
void Config::load_config(const std::string& config_file) {
    std::ifstream config_stream(config_file, std::ifstream::binary);
    if (!config_stream.is_open()) {
        std::cerr << "Failed to load config file: " << config_file << std::endl;
        return;
    }

    // Sử dụng JsonCpp modern API để phân tích JSON an toàn
    Json::CharReaderBuilder builder;
    std::string errs;
    bool ok = Json::parseFromStream(builder, config_stream, &config_data_, &errs);
    if (!ok) {
        std::cerr << "Failed to parse config file: " << config_file << "\n"
                  << errs << std::endl;
    }
}

// Hàm lấy giá trị cấu hình từ JSON
std::string Config::get_config_value(const std::string& key) {
    if (config_data_.isMember(key)) {
        return config_data_[key].asString();  // Trả về giá trị của key tương ứng từ JSON
    } else {
        std::cerr << "Key not found: " << key << std::endl;
        return "";
    }
}

// Hàm lấy chuỗi kết nối server (ví dụ PostgreSQL hoặc địa chỉ UNIX socket)
std::string Config::get_server_address() {
    // Kiểm tra nếu key "server_address" tồn tại trong JSON
    return get_config_value("server_address");
}

// Hàm lấy thông tin về cổng UNIX socket
std::string Config::get_socket_path() {
    // Kiểm tra nếu key "socket_path" tồn tại trong JSON
    return get_config_value("socket_path");
}
