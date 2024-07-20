/*************************************************************************
 * @file    logger.h
 * @brief   包含日志组件spdlog的常用接口
 * @author  stanjiang
 * @date    2024-07-10
 * @copyright
***/

#ifndef _USERS_JIANGPENG_CODE_TRADING_PLATFORM_COMMON_LOGGER_H_
#define _USERS_JIANGPENG_CODE_TRADING_PLATFORM_COMMON_LOGGER_H_

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
    static void log(LogLevel level, const std::string& message);

    template<typename... Args>
    static void log(LogLevel level, const char* fmt, const Args&... args);

 private:
    static std::shared_ptr<spdlog::logger> logger;
    static spdlog::level::level_enum getSpdlogLevel(LogLevel level);
};

template<typename... Args>
void Logger::log(LogLevel level, const char* fmt, const Args&... args) {
    if (!logger) {
        throw std::runtime_error("Logger not initialized. Call Logger::init() first.");
    }
    logger->log(getSpdlogLevel(level), fmt, args...);
}

#endif  // _USERS_JIANGPENG_CODE_TRADING_PLATFORM_COMMON_LOGGER_H_

