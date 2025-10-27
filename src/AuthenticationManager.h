#ifndef AUTHENTICATION_MANAGER_H
#define AUTHENTICATION_MANAGER_H

#include <string>
#include <unordered_map>
#include <vector>

class AuthenticationManager {
private:
    std::unordered_map<std::string, std::string> userDatabase;
    std::string dbFilePath;
    
    bool loadUserDatabase();
    std::string computeSHA1Hash(const std::string& data);
    std::string hexEncode(const std::vector<unsigned char>& data);
    bool validateHexString(const std::string& str, size_t expectedLength);

public:
    // Добавляем константы здесь
    static constexpr const char* DEFAULT_USER_DB = "config/scale.conf";
    static constexpr size_t SALT16_SIZE = 16;
    
    AuthenticationManager(const std::string& dbFile = "");
    bool initialize();
    bool authenticate(const std::string& login, const std::string& salt, const std::string& clientHash);
    size_t getUserCount() const { return userDatabase.size(); }
};

#endif
