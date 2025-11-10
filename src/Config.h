#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>

class Config {
public:
    using data_t = int16_t;  // Для варианта 63 используем int16_t
    
    // Константы конфигурации сервера
    static constexpr uint16_t DEFAULT_PORT = 33333;
    static constexpr const char* DEFAULT_CONFIG_FILE = "config/scale.conf";
    static constexpr const char* DEFAULT_LOG_FILE = "log/server.log";
    
    // Константы аутентификации
    static constexpr int SALT_LENGTH = 16;        // Длина соли в шестнадцатеричных символах
    static constexpr int HASH_LENGTH = 40;        // Длина хеша SHA-1 в шестнадцатеричных символах
    static constexpr int LOGIN_LENGTH = 32;       // Максимальная длина логина
};

#endif
