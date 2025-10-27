#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

class Logger {
private:
    std::ofstream logFile;
    std::mutex logMutex;
    std::string logFilePath;
    bool enabled = true;

    std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

public:
    Logger() = default;
    
    bool initialize(const std::string& filePath) {
        logFilePath = filePath;
        logFile.open(filePath, std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "Ошибка: Не удалось открыть файл лога: " << filePath << std::endl;
            return false;
        }
        log("Logger initialized", "INFO");
        return true;
    }

    void log(const std::string& message, const std::string& level = "INFO") {
        if (!enabled) return;
        
        std::lock_guard<std::mutex> lock(logMutex);
        std::string logEntry = "[" + getCurrentTime() + "] [" + level + "] " + message;
        
        // Вывод в консоль
        std::cout << logEntry << std::endl;
        
        // Запись в файл
        if (logFile.is_open()) {
            logFile << logEntry << std::endl;
            logFile.flush();
        }
    }

    void error(const std::string& message) {
        log(message, "ERROR");
    }

    void warning(const std::string& message) {
        log(message, "WARNING");
    }

    void info(const std::string& message) {
        log(message, "INFO");
    }

    void debug(const std::string& message) {
        log(message, "DEBUG");
    }

    ~Logger() {
        if (logFile.is_open()) {
            log("Logger shutdown", "INFO");
            logFile.close();
        }
    }
};

#endif
