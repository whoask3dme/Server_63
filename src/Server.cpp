#include "Server.h"
#include "Logger.h"
#include "ClientSession.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <memory>

Server::Server(uint16_t port, const std::string& configFile, std::shared_ptr<Logger> logger)
    : port_(port), configFile_(configFile), logger_(logger), running_(false), serverSocket_(-1) {
    
    if (logger_ == nullptr) {
        throw std::invalid_argument("Логгер не может быть nullptr");
    }
}

void Server::run() {
    try {
        createSocket();
        setupAddress();
        bindSocket();
        startListening();
        mainLoop();
    } catch (const std::exception& e) {
        logger_->error("Ошибка сервера: " + std::string(e.what()));
        throw;
    }
}

void Server::createSocket() {
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ == -1) {
        throw std::runtime_error("Не удалось создать сокет: " + std::string(strerror(errno)));
    }
    
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(serverSocket_);
        throw std::runtime_error("Не удалось установить опции сокета: " + std::string(strerror(errno)));
    }
}

void Server::setupAddress() {
    serverAddress_.sin_family = AF_INET;
    serverAddress_.sin_addr.s_addr = INADDR_ANY;
    serverAddress_.sin_port = htons(port_);
}

void Server::bindSocket() {
    if (bind(serverSocket_, (struct sockaddr*)&serverAddress_, sizeof(serverAddress_)) < 0) {
        close(serverSocket_);
        throw std::runtime_error("Не удалось привязать сокет: " + std::string(strerror(errno)));
    }
}

void Server::startListening() {
    if (listen(serverSocket_, 10) < 0) {
        close(serverSocket_);
        throw std::runtime_error("Не удалось начать прослушивание сокета: " + std::string(strerror(errno)));
    }
    
    logger_->info("Сервер запущен и прослушивает порт " + std::to_string(port_));
    running_ = true;
}

void Server::mainLoop() {
    while (running_) {
        try {
            sockaddr_in clientAddress{};
            socklen_t clientAddrLen = sizeof(clientAddress);
            
            int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddress, &clientAddrLen);
            if (clientSocket < 0) {
                if (running_) {
                    throw std::runtime_error("Не удалось принять клиентское соединение: " + std::string(strerror(errno)));
                }
                continue;
            }
            
            // Создаем ClientSession и обрабатываем клиента
            auto clientSession = std::make_unique<ClientSession>(clientSocket, clientAddress, *logger_);
            handleClient(std::move(clientSession));
            
        } catch (const std::exception& e) {
            logger_->error("Ошибка в основном цикле: " + std::string(e.what()));
        }
    }
}

void Server::handleClient(std::unique_ptr<ClientSession> clientSession) {
    try {
        clientSession->handle();
    } catch (const std::exception& e) {
        logger_->error("Ошибка обработки клиента: " + std::string(e.what()));
    }
}

void Server::stop() {
    running_ = false;
    if (serverSocket_ != -1) {
        close(serverSocket_);
        serverSocket_ = -1;
    }
}

Server::~Server() {
    stop();
}
