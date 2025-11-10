#include <iostream>
#include <boost/program_options.hpp>
#include <exception>
#include "Server.h"
#include "Config.h"
#include "Logger.h"

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
    try {
        // Опции командной строки
        po::options_description desc("Опции сервера");
        desc.add_options()
            ("help,h", "Показать справку")
            ("port,p", po::value<uint16_t>()->default_value(Config::DEFAULT_PORT), "Номер порта")
            ("config,c", po::value<std::string>()->default_value(Config::DEFAULT_CONFIG_FILE), "Файл конфигурации")
            ("log,l", po::value<std::string>()->default_value(Config::DEFAULT_LOG_FILE), "Файл лога");
        
        po::variables_map vm;
        
        // ПАРАЛЛЕЛЬНАЯ обработка параметров
        po::store(po::parse_command_line(argc, argv, desc), vm);
        
        // Сначала проверяем help ДО notify, чтобы избежать исключений
        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }
        
        // Теперь выполняем проверки
        po::notify(vm);
        
        // Получение параметров
        uint16_t port = vm["port"].as<uint16_t>();
        std::string configFile = vm["config"].as<std::string>();
        std::string logFile = vm["log"].as<std::string>();
        
        // Запуск сервера - создаем и инициализируем логгер
        auto logger = std::make_shared<Logger>();
        if (!logger->initialize(logFile)) {
            std::cerr << "Не удалось инициализировать логгер" << std::endl;
            return 1;
        }
        
        Server server(port, configFile, logger);
        server.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
