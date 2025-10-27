#include "Server.h"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <signal.h>

Server::Server() 
    : serverPort(Config::DEFAULT_PORT)
    , userDBFile(Config::DEFAULT_USER_DB)
    , logFile(Config::DEFAULT_LOG_FILE)
    , running(false) {
}

Server::~Server() {
    if (serverSocket != -1) {
        close(serverSocket);
    }
}

void Server::showHelp() {
    std::cout << "Использование: server [OPTIONS]\n\n"
              << "Опции:\n"
              << " -h, --help         Показать эту справку\n"
              << " -p, --port PORT    Порт сервера (по умолчанию: " << Config::DEFAULT_PORT << ") \n"
              << " -d, --database FILE Файл базы пользователей (по умолчанию: " << Config::DEFAULT_USER_DB << ") \n"
              << " -l, --log FILE     Файл журнала (по умолчанию: " << Config::DEFAULT_LOG_FILE << ") \n\n"
              << "Примеры:\n"
              << "  server -p 44444 -d my_users.conf\n"
              << "  server --port 33333 --log /var/log/server.log\n";
}

bool Server::parseArguments(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            showHelp();
            return false;
        }
        else if (arg == "-p" || arg == "--port") {
            if (i + 1 < argc) {
                serverPort = std::stoi(argv[++i]);
            } else {
                std::cerr << "Ошибка: не указан порт для параметра " << arg << std::endl;
                return false;
            }
        }
        else if (arg == "-d" || arg == "--database") {
            if (i + 1 < argc) {
                userDBFile = argv[++i];
            } else {
                std::cerr << "Ошибка: не указан файл для параметра " << arg << std::endl;
                return false;
            }
        }
        else if (arg == "-l" || arg == "--log") {
            if (i + 1 < argc) {
                logFile = argv[++i];
            } else {
                std::cerr << "Ошибка: не указан файл для параметра " << arg << std::endl;
                return false;
            }
        }
        else {
            std::cerr << "Неизвестный параметр: " << arg << std::endl;
            showHelp();
            return false;
        }
    }
    return true;
}

bool Server::initializeSocket() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        logger.error("Ошибка создания сокета: " + std::string(strerror(errno)));
        return false;
    }
    
    // Устанавливаем опцию переиспользования адреса
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        logger.error("Ошибка установки SO_REUSEADDR: " + std::string(strerror(errno)));
        close(serverSocket);
        return false;
    }
    
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(serverPort);
    
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        logger.error("Ошибка привязки сокета к порту " + std::to_string(serverPort) + 
                    ": " + std::string(strerror(errno)));
        close(serverSocket);
        return false;
    }
    
    if (listen(serverSocket, 10) < 0) {
        logger.error("Ошибка прослушивания сокета: " + std::string(strerror(errno)));
        close(serverSocket);
        return false;
    }
    
    return true;
}

void Server::handleClient(int clientSocket, struct sockaddr_in clientAddress) {
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddress.sin_addr), clientIP, INET_ADDRSTRLEN);
    int clientPort = ntohs(clientAddress.sin_port);
    
    logger.info("Обработка клиента: " + std::string(clientIP) + ":" + std::to_string(clientPort));
    
    try {
        ClientSession session(clientSocket, clientAddress, logger);
        session.handle();
    } catch (const std::exception& e) {
        logger.error("Ошибка в обработке клиента " + std::string(clientIP) + ": " + e.what());
    }
    
    close(clientSocket);
    logger.info("Завершена обработка клиента: " + std::string(clientIP) + ":" + std::to_string(clientPort));
}

int Server::run(int argc, char* argv[]) {
    if (!parseArguments(argc, argv)) {
        return 1;
    }
    
    // Инициализируем логгер
    if (!logger.initialize(logFile)) {
        std::cerr << "Ошибка инициализации логгера" << std::endl;
        return 1;
    }
    
    logger.info("Запуск сервера...");
    logger.info("Порт: " + std::to_string(serverPort));
    logger.info("База пользователей: " + userDBFile);
    logger.info("Файл лога: " + logFile);
    
    if (!initializeSocket()) {
        return 1;
    }
    
    running = true;
    logger.info("Сервер запущен и слушает порт " + std::to_string(serverPort));
    
    // Игнорируем SIGPIPE чтобы сервер не падал при разрыве соединения
    signal(SIGPIPE, SIG_IGN);
    
    while (running) {
        struct sockaddr_in clientAddress;
        socklen_t clientAddrLen = sizeof(clientAddress);
        
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddrLen);
        if (clientSocket < 0) {
            if (running) {
                logger.error("Ошибка принятия соединения: " + std::string(strerror(errno)));
            }
            continue;
        }
        
        // Обрабатываем клиента в отдельном потоке
        std::thread clientThread(&Server::handleClient, this, clientSocket, clientAddress);
        clientThread.detach();
    }
    
    logger.info("Сервер остановлен");
    return 0;
}
