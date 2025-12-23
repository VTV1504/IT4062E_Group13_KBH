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
    // Không có resource đặc biệt cần giải phóng
}

// Hàm đọc cấu hình từ file JSON
void Config::load_config(const std::string& config_file) {
    std::ifstream config_stream(config_file, std::ifstream::binary);
    if (!config_stream.is_open()) {
        std::cerr << "Failed to load config file: " << config_file << std::endl;
        return;
    }

    Json::CharReaderBuilder builder;
    std::string errs;
    bool ok = Json::parseFromStream(builder, config_stream, &config_data_, &errs);
    if (!ok) {
        std::cerr << "Failed to parse config file: " << config_file << "\n"
                  << errs << std::endl;
    }
}

// Hàm lấy giá trị cấu hình từ JSON dưới dạng string
std::string Config::get_config_value(const std::string& key) {
    if (config_data_.isMember(key)) {
        return config_data_[key].asString();
    } else {
        std::cerr << "Key not found: " << key << std::endl;
        return "";
    }
}

// Hàm lấy chuỗi địa chỉ server (legacy)
std::string Config::get_server_address() {
    return get_config_value("server_address");
}

// Hàm lấy thông tin về đường dẫn UNIX socket (nếu dùng)
std::string Config::get_socket_path() {
    return get_config_value("socket_path");
}

// ===== Thêm cho TCP server =====

// Lấy IP server cho TCP
std::string Config::get_server_ip() {
    if (config_data_.isMember("server_ip")) {
        return config_data_["server_ip"].asString();
    }
    // fallback: dùng server_address nếu không có server_ip
    return get_server_address();
}

// Lấy port server cho TCP, mặc định 5000 nếu thiếu
int Config::get_server_port() {
    if (config_data_.isMember("server_port")) {
        const auto& v = config_data_["server_port"];
        if (v.isInt()) {
            return v.asInt();
        }
        if (v.isString()) {
            try {
                return std::stoi(v.asString());
            } catch (...) {
                std::cerr << "Invalid server_port value, fallback 5000\n";
            }
        }
    }
    return 5000;
}
