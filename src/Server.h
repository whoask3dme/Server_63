#ifndef SERVER_H
#define SERVER_H

#include <cstdint>
#include <string>
#include <memory>
#include <netinet/in.h>

class Logger;
class ClientSession;

class Server {
public:
    Server(uint16_t port, const std::string& configFile, std::shared_ptr<Logger> logger);
    ~Server();
    
    void run();          // Запуск сервера
    void stop();         // Остановка сервера

private:
    void createSocket();     // Создание сокета
    void setupAddress();     // Настройка адреса
    void bindSocket();       // Привязка сокета
    void startListening();   // Начало прослушивания
    void mainLoop();         // Основной цикл обработки
    void handleClient(std::unique_ptr<ClientSession> clientSession); // Обработка клиента
    
    uint16_t port_;                    // Порт сервера
    std::string configFile_;           // Файл конфигурации
    std::shared_ptr<Logger> logger_;   // Логгер
    bool running_;                     // Флаг работы сервера
    int serverSocket_;                 // Серверный сокет
    sockaddr_in serverAddress_;        // Адрес сервера
};

#endif
