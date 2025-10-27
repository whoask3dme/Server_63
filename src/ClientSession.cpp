#include "ClientSession.h"
#include <iostream>
#include <arpa/inet.h>
#include <cstring>

ClientSession::ClientSession(int socket, const struct sockaddr_in& address, Logger& logger)
    : clientSocket(socket), clientAddress(address), logger(logger) {
    
    authManager = std::make_unique<AuthenticationManager>();
    dataProcessor = std::make_unique<DataProcessor>();
    
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddress.sin_addr), clientIP, INET_ADDRSTRLEN);
    
    logger.info("Создана сессия для клиента: " + std::string(clientIP) + ":" + 
                std::to_string(ntohs(clientAddress.sin_port)));
}

ClientSession::~ClientSession() {
    closeConnection();
}

void ClientSession::closeConnection() {
    if (clientSocket != -1) {
        close(clientSocket);
        clientSocket = -1;
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

bool ClientSession::authenticateClient() {
    // Получаем размер сообщения аутентификации
    uint32_t msgSize;
    if (!receiveAll(&msgSize, sizeof(msgSize))) {
        logger.error("Ошибка получения размера сообщения аутентификации");
        return false;
    }
    msgSize = ntohl(msgSize);
    
    // Получаем само сообщение
    std::vector<char> authBuffer(msgSize + 1);
    if (!receiveAll(authBuffer.data(), msgSize)) {
        logger.error("Ошибка получения сообщения аутентификации");
        return false;
    }
    authBuffer[msgSize] = '\0';
    std::string authMessage(authBuffer.data());
    
    // Парсим сообщение: LOGIN + SALT(16 hex) + HASH(40 hex)
    if (authMessage.length() < 1 + AuthenticationManager::SALT16_SIZE + 40) {
        logger.error("Слишком короткое сообщение аутентификации");
        return false;
    }
    
    size_t loginSize = authMessage.length() - AuthenticationManager::SALT16_SIZE - 40;
    std::string login = authMessage.substr(0, loginSize);
    std::string salt = authMessage.substr(loginSize, AuthenticationManager::SALT16_SIZE);
    std::string clientHash = authMessage.substr(loginSize + AuthenticationManager::SALT16_SIZE, 40);
    
    // Аутентифицируем
    if (!authManager->authenticate(login, salt, clientHash)) {
        std::string errorMsg = "ERR";
        sendAll(errorMsg.c_str(), errorMsg.length());
        return false;
    }
    
    // Отправляем подтверждение
    std::string successMsg = "OK";
    if (!sendAll(successMsg.c_str(), successMsg.length())) {
        logger.error("Ошибка отправки подтверждения аутентификации");
        return false;
    }
    
    authenticated = true;
    logger.info("Клиент аутентифицирован: " + login);
    return true;
}

bool ClientSession::processDataExchange() {
    if (!authenticated) {
        logger.error("Попытка обмена данными без аутентификации");
        return false;
    }
    
    // Получаем количество векторов
    uint32_t vectorCount;
    if (!receiveAll(&vectorCount, sizeof(vectorCount))) {
        logger.error("Ошибка получения количества векторов");
        return false;
    }
    vectorCount = ntohl(vectorCount);
    
    std::vector<std::vector<Config::data_t>> vectors;
    
    // Получаем каждый вектор
    for (uint32_t i = 0; i < vectorCount; ++i) {
        // Получаем размер вектора
        uint32_t vectorSize;
        if (!receiveAll(&vectorSize, sizeof(vectorSize))) {
            logger.error("Ошибка получения размера вектора " + std::to_string(i));
            return false;
        }
        vectorSize = ntohl(vectorSize);
        
        // Получаем данные вектора
        std::vector<Config::data_t> vector(vectorSize);
        if (!receiveAll(vector.data(), vectorSize * sizeof(Config::data_t))) {
            logger.error("Ошибка получения данных вектора " + std::to_string(i));
            return false;
        }
        
        // Конвертируем из сетевого порядка байт
        for (auto& value : vector) {
            value = ntohs(value);
        }
        
        vectors.push_back(vector);
    }
    
    // Обрабатываем векторы
    auto results = dataProcessor->processVectors(vectors);
    
    // Отправляем результаты
    uint32_t resultCount = htonl(results.size());
    if (!sendAll(&resultCount, sizeof(resultCount))) {
        logger.error("Ошибка отправки количества результатов");
        return false;
    }
    
    for (auto result : results) {
        Config::data_t netResult = htons(result);
        if (!sendAll(&netResult, sizeof(netResult))) {
            logger.error("Ошибка отправки результата");
            return false;
        }
    }
    
    logger.info("Обработано векторов: " + std::to_string(vectorCount) + 
                ", результатов: " + std::to_string(results.size()));
    return true;
}

void ClientSession::handle() {
    logger.info("Начало обработки клиентской сессии");
    
    try {
        if (!authManager->initialize()) {
            logger.error("Ошибка инициализации менеджера аутентификации");
            return;
        }
        
        if (!authenticateClient()) {
            logger.error("Ошибка аутентификации клиента");
            return;
        }
        
        if (!processDataExchange()) {
            logger.error("Ошибка обработки данных");
            return;
        }
        
        logger.info("Сессия успешно завершена");
        
    } catch (const std::exception& e) {
        logger.error("Исключение в клиентской сессии: " + std::string(e.what()));
    }
}
