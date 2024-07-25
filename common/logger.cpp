#include "logger.h"
#include <iostream>

std::shared_ptr<spdlog::logger> Logger::logger;

void Logger::init(const std::string& logFilePath) {
    try {
        logger = spdlog::basic_logger_mt("file_logger", logFilePath);
        logger->set_level(spdlog::level::debug);  // Set global log level to debug
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %l: %v");
        spdlog::flush_every(std::chrono::seconds(3));  // Flush every 3 seconds
    }
    catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
        throw;
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (!logger) {
        throw std::runtime_error("Logger not initialized. Call Logger::init() first.");
    }
    logger->log(getSpdlogLevel(level), message);
}

spdlog::level::level_enum Logger::getSpdlogLevel(LogLevel level) {
    switch (level) {
        case INFO:
            return spdlog::level::info;
        case DEBUG:
            return spdlog::level::debug;
        case ERROR:
            return spdlog::level::err;
        default:
            return spdlog::level::info;
    }
}