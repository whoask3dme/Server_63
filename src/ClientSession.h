#ifndef CLIENT_SESSION_H
#define CLIENT_SESSION_H

#include "Config.h"
#include "AuthenticationManager.h"
#include "Logger.h"
#include "DataProcessor.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <memory>
#include <thread>

class ClientSession {
private:
    int clientSocket;
    struct sockaddr_in clientAddress;
    std::unique_ptr<AuthenticationManager> authManager;
    std::unique_ptr<DataProcessor> dataProcessor;
    Logger& logger;
    bool authenticated = false;

    bool receiveAll(void* buffer, size_t length);
    bool sendAll(const void* buffer, size_t length);
    bool authenticateClient();
    bool processDataExchange();
    void closeConnection();

public:
    ClientSession(int socket, const struct sockaddr_in& address, Logger& logger);
    ~ClientSession();
    void handle();
};

#endif
