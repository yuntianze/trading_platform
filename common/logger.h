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

enum LogLevel {
    INFO,
    DEBUG,
    ERROR
};

class Logger {
 public:
    static void init(const std::string& logFilePath);

    template<typename... Args>
    static void log(LogLevel level, const char* file, int line, const char* fmt, const Args&... args);

 private:
    static std::shared_ptr<spdlog::logger> logger;
    static spdlog::level::level_enum getSpdlogLevel(LogLevel level);
};

template<typename... Args>
void Logger::log(LogLevel level, const char* file, int line, const char* fmt, const Args&... args) {
    if (!logger) {
        throw std::runtime_error("Logger not initialized. Call Logger::init() first.");
    }
    std::string format = fmt::format("[{}:{}] {}", file, line, fmt);
    logger->log(getSpdlogLevel(level), format, args...);
}

#define LOG(level, ...) Logger::log(level, __FILE__, __LINE__, __VA_ARGS__)

#endif  // _TRADING_PLATFORM_COMMON_LOGGER_H_