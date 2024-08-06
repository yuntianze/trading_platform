// config_manager.cpp

#include "config_manager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include "logger.h"

ConfigManager& ConfigManager::instance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::load_config(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG(ERROR, "Failed to open configuration file: {}", filename);
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Remove leading and trailing whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Parse key-value pairs
        size_t delimiter_pos = line.find('=');
        if (delimiter_pos != std::string::npos) {
            std::string key = line.substr(0, delimiter_pos);
            std::string value = line.substr(delimiter_pos + 1);

            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            config_[key] = value;
        }
    }

    LOG(INFO, "Configuration loaded successfully from file: {}", filename);
    return true;
}

std::string ConfigManager::get_string(const std::string& key, const std::string& default_value) const {
    auto it = config_.find(key);
    if (it != config_.end()) {
        return it->second;
    }
    return default_value;
}

int ConfigManager::get_int(const std::string& key, int default_value) const {
    auto it = config_.find(key);
    if (it != config_.end()) {
        try {
            return std::stoi(it->second);
        } catch (const std::exception& e) {
            LOG(ERROR, "Failed to convert '{}' to integer for key '{}': {}", it->second, key, e.what());
        }
    }
    return default_value;
}

bool ConfigManager::get_bool(const std::string& key, bool default_value) const {
    auto it = config_.find(key);
    if (it != config_.end()) {
        std::string value = it->second;
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        if (value == "true" || value == "1" || value == "yes" || value == "on") {
            return true;
        } else if (value == "false" || value == "0" || value == "no" || value == "off") {
            return false;
        }
        LOG(ERROR, "Invalid boolean value '{}' for key '{}'", it->second, key);
    }
    return default_value;
}