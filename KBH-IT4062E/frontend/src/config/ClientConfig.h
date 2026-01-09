#pragma once
#include <string>

struct ClientConfig {
    std::string server_ip = "127.0.0.1";
    int server_port = 5000;
    
    // Load from config file
    static ClientConfig load(const std::string& config_path = "../config/server_config.json");
};
