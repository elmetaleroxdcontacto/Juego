#pragma once

#include <string>
#include <iostream>
#include <fstream>

class Logger {
public:
    enum class Level { Info, Warning, Error };

    static Logger& getInstance();
    void setLogFile(const std::string& filePath);
    void log(Level level, const std::string& message);

private:
    Logger();
    std::ofstream logFile;
};

#define LOG_INFO(msg) Logger::getInstance().log(Logger::Level::Info, msg)
#define LOG_WARNING(msg) Logger::getInstance().log(Logger::Level::Warning, msg)
#define LOG_ERROR(msg) Logger::getInstance().log(Logger::Level::Error, msg)