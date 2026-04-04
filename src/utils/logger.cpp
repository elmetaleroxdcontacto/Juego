#include "utils/logger.h"
#include <ctime>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {}

void Logger::setLogFile(const std::string& filePath) {
    if (logFile.is_open()) logFile.close();
    logFile.open(filePath, std::ios::app);
}

void Logger::log(Level level, const std::string& message) {
    std::string levelStr;
    switch (level) {
        case Level::Info: levelStr = "INFO"; break;
        case Level::Warning: levelStr = "WARNING"; break;
        case Level::Error: levelStr = "ERROR"; break;
    }
    std::time_t now = std::time(nullptr);
    char timeStr[20];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    std::string logEntry = std::string(timeStr) + " [" + levelStr + "] " + message + "\n";
    std::cout << logEntry;
    if (logFile.is_open()) {
        logFile << logEntry;
        logFile.flush();
    }
}