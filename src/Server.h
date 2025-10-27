#ifndef SERVER_H
#define SERVER_H

#include "Config.h"
#include "AuthenticationManager.h"
#include "Logger.h"
#include "ClientSession.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <memory>
#include <thread>
#include <vector>
#include <atomic>

class Server {
private:
    int serverSocket;
    int serverPort;
    std::string userDBFile;
    std::string logFile;
    std::atomic<bool> running;
    Logger logger;
    
    bool parseArguments(int argc, char* argv[]);
    bool initializeSocket();
    void showHelp();
    void handleClient(int clientSocket, struct sockaddr_in clientAddress);

public:
    Server();
    ~Server();
    int run(int argc, char* argv[]);
};

#endif
