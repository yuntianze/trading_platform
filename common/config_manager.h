/*************************************************************************
 * @file    config_manager.h
 * @brief   Configuration manager class declaration
 * @author  stanjiang
 * @date    2024-08-06
 * @copyright
***/

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <unordered_map>

class ConfigManager {
public:
    // Singleton instance getter
    static ConfigManager& instance();

    // Load configuration from file
    bool load_config(const std::string& filename);

    // Get a string value from the configuration
    std::string get_string(const std::string& key, const std::string& default_value = "") const;

    // Get an integer value from the configuration
    int get_int(const std::string& key, int default_value = 0) const;

    // Get a boolean value from the configuration
    bool get_bool(const std::string& key, bool default_value = false) const;

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    std::unordered_map<std::string, std::string> config_;
};

#endif // CONFIG_MANAGER_H