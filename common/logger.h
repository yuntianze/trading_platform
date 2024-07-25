/*************************************************************************
 * @file    logger.h
 * @brief   Contains common interfaces for the logging component spdlog
 * @author  stanjiang
 * @date    2024-07-10
 * @copyright
***/

#ifndef _TRADING_PLATFORM_COMMON_LOGGER_H_
#define _TRADING_PLATFORM_COMMON_LOGGER_H_

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <string>

// Enum for log levels
enum LogLevel {
    INFO,
    DEBUG,
    ERROR
};

class Logger {
 public:
    /**
     * @brief Initialize the logger
     * @param logFilePath Path to the log file
     */
    static void init(const std::string& logFilePath);

    /**
     * @brief Log a message with the specified log level
     * @param level Log level
     * @param message Message to log
     */
    static void log(LogLevel level, const std::string& message);

    /**
     * @brief Log a formatted message with the specified log level
     * @param level Log level
     * @param fmt Format string
     * @param args Arguments for the format string
     */
    template<typename... Args>
    static void log(LogLevel level, const char* fmt, const Args&... args);

 private:
    static std::shared_ptr<spdlog::logger> logger;
    
    /**
     * @brief Convert LogLevel to spdlog::level::level_enum
     * @param level LogLevel to convert
     * @return Corresponding spdlog::level::level_enum
     */
    static spdlog::level::level_enum getSpdlogLevel(LogLevel level);
};

template<typename... Args>
void Logger::log(LogLevel level, const char* fmt, const Args&... args) {
    if (!logger) {
        throw std::runtime_error("Logger not initialized. Call Logger::init() first.");
    }
    logger->log(getSpdlogLevel(level), fmt, args...);
}

#endif  // _TRADING_PLATFORM_COMMON_LOGGER_H_