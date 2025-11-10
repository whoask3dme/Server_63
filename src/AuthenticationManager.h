#ifndef AUTHENTICATIONMANAGER_H
#define AUTHENTICATIONMANAGER_H

#include <string>
#include <unordered_map>
#include <memory>

class Logger;

class AuthenticationManager {
public:
    AuthenticationManager(const std::string& configFile, std::shared_ptr<Logger> logger);
    bool authenticate(const std::string& authData); // 

private:
    void loadUserDatabase(); // 
    bool verifyHash(const std::string& password, const std::string& salt, const std::string& receivedHash);
    std::string computeSHA1(const std::string& data);
    
    std::unordered_map<std::string, std::string> userDatabase_;
    std::string configFile_;
    std::shared_ptr<Logger> logger_;
};

#endif
