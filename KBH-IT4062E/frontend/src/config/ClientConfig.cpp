#include "ClientConfig.h"
#include <jsoncpp/json/json.h>
#include <fstream>
#include <iostream>

ClientConfig ClientConfig::load(const std::string& config_path) {
    ClientConfig cfg;
    
    std::ifstream file(config_path);
    if (!file.is_open()) {
        std::cerr << "[ClientConfig] Warning: Could not open " << config_path 
                  << ", using defaults (127.0.0.1:5000)\n";
        return cfg;
    }
    
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    
    if (!Json::parseFromStream(builder, file, &root, &errs)) {
        std::cerr << "[ClientConfig] JSON parse error: " << errs << "\n";
        return cfg;
    }
    
    if (root.isMember("server_ip") && root["server_ip"].isString()) {
        cfg.server_ip = root["server_ip"].asString();
    }
    
    if (root.isMember("server_port") && root["server_port"].isInt()) {
        cfg.server_port = root["server_port"].asInt();
    }
    
    std::cout << "[ClientConfig] Loaded config: " << cfg.server_ip << ":" << cfg.server_port << "\n";
    
    return cfg;
}
