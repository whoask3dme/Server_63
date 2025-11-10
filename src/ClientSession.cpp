#include "ClientSession.h"
#include "Logger.h"
#include "AuthenticationManager.h"
#include "DataProcessor.h"
#include "Config.h"
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <arpa/inet.h>

ClientSession::ClientSession(int socket, const struct sockaddr_in& address, Logger& logger)
    : clientSocket(socket), clientAddress(address), logger(logger) {
    
    // Передаем ссылку на логгер в AuthenticationManager
    authManager = std::make_unique<AuthenticationManager>("config/scale.conf", 
        std::shared_ptr<Logger>(&logger, [](Logger*){}));
    dataProcessor = std::make_unique<DataProcessor>();
}

ClientSession::~ClientSession() {
    closeConnection();
}

void ClientSession::handle() {
    try {
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddress.sin_addr), ipStr, INET_ADDRSTRLEN);
        logger.info("Клиент подключен: " + std::string(ipStr) + ":" + 
                   std::to_string(ntohs(clientAddress.sin_port)));
        
        // Аутентификация
        if (!authenticateClient()) {
            throw std::runtime_error("Аутентификация не удалась");
        }
        
        // Обработка данных
        if (!processDataExchange()) {
            throw std::runtime_error("Ошибка обработки данных");
        }
        
        logger.info("Сессия завершена успешно");
        
    } catch (const std::exception& e) {
        logger.error("Ошибка сессии: " + std::string(e.what()));
        throw;
    }
}

bool ClientSession::authenticateClient() {
    try {
        // Чтение аутентификационных данных
        char authBuffer[1024];
        if (!receiveAll(authBuffer, Config::LOGIN_LENGTH + Config::SALT_LENGTH + Config::HASH_LENGTH)) {
            throw std::runtime_error("Не удалось прочитать аутентификационные данные");
        }
        
        std::string authData(authBuffer, Config::LOGIN_LENGTH + Config::SALT_LENGTH + Config::HASH_LENGTH);
        
        // Аутентификация
        bool result = authManager->authenticate(authData);
        
        if (result) {
            const char* okResponse = "OK";
            if (!sendAll(okResponse, 2)) {
                throw std::runtime_error("Не удалось отправить ответ OK");
            }
            logger.info("Аутентификация успешна");
            authenticated = true;
        } else {
            const char* errResponse = "ERR";
            if (!sendAll(errResponse, 3)) {
                throw std::runtime_error("Не удалось отправить ответ ERR");
            }
            logger.warning("Аутентификация не удалась");
        }
        
        return result;
        
    } catch (const std::exception& e) {
        logger.error("Ошибка аутентификации: " + std::string(e.what()));
        return false;
    }
}

bool ClientSession::processDataExchange() {
    if (!authenticated) {
        throw std::runtime_error("Попытка обработки данных без аутентификации");
    }
    
    try {
        // Чтение количества векторов
        uint32_t vectorCount;
        if (!receiveAll(&vectorCount, sizeof(vectorCount))) {
            throw std::runtime_error("Не удалось прочитать количество векторов");
        }
        vectorCount = ntohl(vectorCount);
        
        std::vector<std::vector<Config::data_t>> vectors;
        vectors.reserve(vectorCount);
        
        // Чтение каждого вектора
        for (uint32_t i = 0; i < vectorCount; ++i) {
            // Чтение размера вектора
            uint32_t vectorSize;
            if (!receiveAll(&vectorSize, sizeof(vectorSize))) {
                throw std::runtime_error("Не удалось прочитать размер вектора");
            }
            vectorSize = ntohl(vectorSize);
            
            // Чтение данных вектора
            std::vector<Config::data_t> vector(vectorSize);
            if (!receiveAll(vector.data(), vectorSize * sizeof(Config::data_t))) {
                throw std::runtime_error("Не удалось прочитать данные вектора");
            }
            
            vectors.push_back(std::move(vector));
        }
        
        // Обработка данных
        auto results = dataProcessor->processVectors(vectors);
        
        // Отправка результатов
        uint32_t resultCount = htonl(results.size());
        if (!sendAll(&resultCount, sizeof(resultCount))) {
            throw std::runtime_error("Не удалось отправить количество результатов");
        }
        
        for (const auto& result : results) {
            if (!sendAll(&result, sizeof(result))) {
                throw std::runtime_error("Не удалось отправить результат");
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        logger.error("Ошибка обработки данных: " + std::string(e.what()));
        return false;
    }
}

bool ClientSession::receiveAll(void* buffer, size_t length) {
    char* ptr = static_cast<char*>(buffer);
    size_t totalReceived = 0;
    
    while (totalReceived < length) {
        ssize_t received = recv(clientSocket, ptr + totalReceived, length - totalReceived, 0);
        if (received <= 0) {
            return false;
        }
        totalReceived += received;
    }
    return true;
}

bool ClientSession::sendAll(const void* buffer, size_t length) {
    const char* ptr = static_cast<const char*>(buffer);
    size_t totalSent = 0;
    
    while (totalSent < length) {
        ssize_t sent = send(clientSocket, ptr + totalSent, length - totalSent, 0);
        if (sent <= 0) {
            return false;
        }
        totalSent += sent;
    }
    return true;
}

void ClientSession::closeConnection() {
    if (clientSocket != -1) {
        close(clientSocket);
        clientSocket = -1;
    }
}
