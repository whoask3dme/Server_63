#include "AuthenticationManager.h"
#include "Logger.h"
#include "Config.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <openssl/sha.h>
#include <iomanip>

AuthenticationManager::AuthenticationManager(const std::string& configFile, std::shared_ptr<Logger> logger)
    : configFile_(configFile), logger_(logger) {
    loadUserDatabase();
}

bool AuthenticationManager::authenticate(const std::string& authData) {
    const int MIN_AUTH_DATA_LENGTH = Config::LOGIN_LENGTH + Config::SALT_LENGTH + Config::HASH_LENGTH;
    if (authData.length() < MIN_AUTH_DATA_LENGTH) {
        throw std::invalid_argument("Аутентификационные данные слишком короткие");
    }
    
    try {
        std::string login = authData.substr(0, Config::LOGIN_LENGTH);
        std::string salt = authData.substr(Config::LOGIN_LENGTH, Config::SALT_LENGTH);
        std::string receivedHash = authData.substr(Config::LOGIN_LENGTH + Config::SALT_LENGTH, Config::HASH_LENGTH);
        
        // Удаление пробелов
        login.erase(login.find_last_not_of(" \n\r\t") + 1);
        
        auto userIt = userDatabase_.find(login);
        if (userIt == userDatabase_.end()) {
            throw std::runtime_error("Пользователь не найден: " + login);
        }
        
        return verifyHash(userIt->second, salt, receivedHash);
        
    } catch (const std::exception& e) {
        logger_->error("Ошибка обработки аутентификации: " + std::string(e.what()));
        throw;
    }
}

void AuthenticationManager::loadUserDatabase() {
    std::ifstream file(configFile_);
    if (!file.is_open()) {
        throw std::runtime_error("Не удалось открыть файл базы пользователей: " + configFile_);
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        size_t delimiterPos = line.find(':');
        if (delimiterPos != std::string::npos) {
            std::string login = line.substr(0, delimiterPos);
            std::string password = line.substr(delimiterPos + 1);
            
            login.erase(login.find_last_not_of(" \n\r\t") + 1);
            password.erase(password.find_last_not_of(" \n\r\t") + 1);
            
            if (!login.empty() && !password.empty()) {
                userDatabase_[login] = password;
            }
        }
    }
    
    logger_->info("База пользователей загружена: " + std::to_string(userDatabase_.size()) + " пользователей");
}

bool AuthenticationManager::verifyHash(const std::string& password, const std::string& salt, const std::string& receivedHash) {
    std::string computedHash = computeSHA1(salt + password);
    return (computedHash == receivedHash);
}

std::string AuthenticationManager::computeSHA1(const std::string& data) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), hash);
    
    std::stringstream ss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}
