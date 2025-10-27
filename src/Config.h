#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct Config {
    std::string authFile = "config/scale.conf";
    std::string logFile = "log/server.log";
    int port = 33333;
    
    // Параметры для int16_t
    using data_t = int16_t;
    static constexpr int16_t MAX_VALUE = 32767;
    static constexpr int16_t MIN_VALUE = -32768;
    
    // Параметры аутентификации
    static constexpr size_t SALT_SIZE = 8;
    static constexpr size_t SALT_HEX_SIZE = 16;
    
    // Константы для сервера
    static constexpr int DEFAULT_PORT = 33333;
    static constexpr const char* DEFAULT_USER_DB = "config/scale.conf";
    static constexpr const char* DEFAULT_LOG_FILE = "log/server.log";
};

#endif
