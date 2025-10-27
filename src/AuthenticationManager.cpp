#include "AuthenticationManager.h"
#include "Config.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <algorithm>
#include <cctype>

AuthenticationManager::AuthenticationManager(const std::string& dbFile) 
    : dbFilePath(dbFile.empty() ? DEFAULT_USER_DB : dbFile) {}

bool AuthenticationManager::loadUserDatabase() {
    std::ifstream dbFile(dbFilePath);
    if (!dbFile.is_open()) {
        return false;
    }
    
    userDatabase.clear();
    std::string line;
    int lineNumber = 0;
    
    while (std::getline(dbFile, line)) {
        lineNumber++;
        
        // Пропускаем пустые строки и комментарии
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Разделяем логин и пароль по двоеточию
        size_t delimiterPos = line.find(':');
        if (delimiterPos == std::string::npos || delimiterPos == 0) {
            std::cerr << "Предупреждение: Некорректный формат в строке " << lineNumber 
                      << ": " << line << std::endl;
            continue;
        }
        
        std::string login = line.substr(0, delimiterPos);
        std::string password = line.substr(delimiterPos + 1);
        
        // Удаляем пробелы в начале и конце
        login.erase(0, login.find_first_not_of(" \t"));
        login.erase(login.find_last_not_of(" \t") + 1);
        password.erase(0, password.find_first_not_of(" \t"));
        password.erase(password.find_last_not_of(" \t") + 1);
        
        if (login.empty() || password.empty()) {
            std::cerr << "Предупреждение: Пустой логин или пароль в строке " 
                      << lineNumber << std::endl;
            continue;
        }
        
        // Проверяем уникальность логина
        if (userDatabase.find(login) != userDatabase.end()) {
            std::cerr << "Предупреждение: Дублирующийся логин в строке " 
                      << lineNumber << ": " << login << std::endl;
            continue;
        }
        
        userDatabase[login] = password;
    }
    
    dbFile.close();
    return true;
}

std::string AuthenticationManager::hexEncode(const std::vector<unsigned char>& data) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    for (unsigned char byte : data) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    
    return ss.str();
}

bool AuthenticationManager::validateHexString(const std::string& str, size_t expectedLength) {
    if (str.length() != expectedLength) {
        return false;
    }
    
    // Проверяем, что все символы - допустимые шестнадцатеричные цифры
    for (char c : str) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    
    return true;
}

std::string AuthenticationManager::computeSHA1Hash(const std::string& data) {
    std::vector<unsigned char> hash(SHA_DIGEST_LENGTH);
    
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (!context) {
        throw std::runtime_error("Ошибка создания контекста SHA-1");
    }
    
    try {
        if (EVP_DigestInit_ex(context, EVP_sha1(), nullptr) != 1) {
            throw std::runtime_error("Ошибка инициализации SHA-1");
        }
        
        if (EVP_DigestUpdate(context, data.c_str(), data.length()) != 1) {
            throw std::runtime_error("Ошибка обновления SHA-1");
        }
        
        unsigned int length = 0;
        if (EVP_DigestFinal_ex(context, hash.data(), &length) != 1) {
            throw std::runtime_error("Ошибка финализации SHA-1");
        }
        
        if (length != SHA_DIGEST_LENGTH) {
            throw std::runtime_error("Некорректная длина SHA-1 хэша");
        }
        
        EVP_MD_CTX_free(context);
        return hexEncode(hash);
    }
    catch (...) {
        EVP_MD_CTX_free(context);
        throw;
    }
}

bool AuthenticationManager::initialize() {
    if (!loadUserDatabase()) {
        std::cerr << "Ошибка: Не удалось загрузить базу пользователей из файла: " 
                  << dbFilePath << std::endl;
        return false;
    }
    
    std::cout << "База пользователей загружена: " << userDatabase.size() 
              << " пользователей" << std::endl;
    
    // Выводим список загруженных пользователей (для отладки)
    for (const auto& [login, password] : userDatabase) {
        std::cout << "  " << login << ": " << std::string(password.length(), '*') << std::endl;
    }
    
    return true;
}

bool AuthenticationManager::authenticate(const std::string& login, const std::string& salt, const std::string& clientHash) {
    // Валидация входных данных
    if (login.empty()) {
        std::cerr << "Ошибка аутентификации: Пустой логин" << std::endl;
        return false;
    }
    
    // Проверяем формат соли (16 шестнадцатеричных цифр)
    if (!validateHexString(salt, SALT16_SIZE)) {
        std::cerr << "Ошибка аутентификации: Некорректный формат соли для логина " << login << std::endl;
        return false;
    }
    
    // Проверяем формат хэша (40 шестнадцатеричных цифр для SHA-1)
    if (!validateHexString(clientHash, 40)) {
        std::cerr << "Ошибка аутентификации: Некорректный формат хэша для логина " << login << std::endl;
        return false;
    }
    
    // Ищем пользователя в базе
    auto it = userDatabase.find(login);
    if (it == userDatabase.end()) {
        std::cerr << "Ошибка аутентификации: Пользователь не найден: " << login << std::endl;
        return false;
    }
    
    const std::string& password = it->second;
    
    try {
        // Вычисляем ожидаемый хэш: SHA1(salt + password)
        std::string dataToHash = salt + password;
        std::string computedHash = computeSHA1Hash(dataToHash);
        
        // Сравниваем хэши (без учета регистра, так как hex в верхнем регистре)
        std::string clientHashUpper = clientHash;
        std::transform(clientHashUpper.begin(), clientHashUpper.end(), clientHashUpper.begin(), ::toupper);
        std::string computedHashUpper = computedHash;
        std::transform(computedHashUpper.begin(), computedHashUpper.end(), computedHashUpper.begin(), ::toupper);
        
        bool hashesMatch = (clientHashUpper == computedHashUpper);
        
        if (!hashesMatch) {
            std::cerr << "Ошибка аутентификации: Неверный пароль для пользователя " << login << std::endl;
            std::cerr << "  Ожидаемый хэш: " << computedHashUpper << std::endl;
            std::cerr << "  Полученный хэш: " << clientHashUpper << std::endl;
        }
        
        return hashesMatch;
        
    } catch (const std::exception& e) {
        std::cerr << "Ошибка вычисления хэша для пользователя " << login << ": " << e.what() << std::endl;
        return false;
    }
}
