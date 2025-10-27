# Компилятор и флаги
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
LIBS = -lssl -lcrypto -lpthread

# Компилятор и флаги
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
LIBS = -lssl -lcrypto -lpthread

# Директории
SRCDIR = src
OBJDIR = obj
BINDIR = bin
CONFIGDIR = config
LOGDIR = log

# Исходные файлы
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
TARGET = $(BINDIR)/server

# Основная цель
$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LIBS)

# Компиляция объектных файлов
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Создание директорий
$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

$(LOGDIR):
	mkdir -p $(LOGDIR)

# Дополнительные цели
all: $(TARGET)

# Запуск сервера с параметрами по умолчанию
run: all $(LOGDIR)
	./$(BINDIR)/server

# Запуск с конкретным портом
run-port: all $(LOGDIR)
	./$(BINDIR)/server -p 44444

# Очистка
clean:
	rm -rf $(OBJDIR) $(BINDIR)

distclean: clean
	rm -rf $(LOGDIR)/*

# Установка зависимостей (для Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install libssl-dev build-essential

# Создание конфигурационного файла
create-config:
	@echo "Создание файла конфигурации..."
	@mkdir -p $(CONFIGDIR)
	@echo "# Файл базы пользователей сервера" > $(CONFIGDIR)/scale.conf
	@echo "# Формат: логин:пароль" >> $(CONFIGDIR)/scale.conf
	@echo "" >> $(CONFIGDIR)/scale.conf
	@echo "user:P@ssWOrd" >> $(CONFIGDIR)/scale.conf
	@echo "testuser:testpass123" >> $(CONFIGDIR)/scale.conf
	@echo "admin:admin123" >> $(CONFIGDIR)/scale.conf
	@echo "user1:secret123" >> $(CONFIGDIR)/scale.conf
	@echo "ibst128:SuperSecretPassword" >> $(CONFIGDIR)/scale.conf
	@echo "Файл конфигурации создан: $(CONFIGDIR)/scale.conf"

# Отладочная сборка
debug: CXXFLAGS += -DDEBUG -Og
debug: $(TARGET)

# Показать структуру проекта
tree:
	@echo "Структура проекта:"
	@echo "курсовая_сервер/"
	@echo "├── src/                    # Исходные файлы"
	@echo "├── bin/                    # Исполняемые файлы"
	@echo "├── obj/                    # Объектные файлы"
	@echo "├── config/                 # Конфигурационные файлы"
	@echo "│   └── scale.conf"
	@echo "├── log/                    # Директория для логов"
	@echo "├── tests/                  # Тесты"
	@echo "├── Makefile"
	@echo "└── README.md"

# Тестирование сервера (запуск в фоне)
test-server: all $(LOGDIR)
	@echo "Запуск сервера для тестирования на порту 33333..."
	./$(BINDIR)/server -p 33333 &
	@sleep 2

# Остановка тестового сервера
stop-test:
	@pkill server || true
	@echo "Сервер остановлен"

# Проверка порта
check-port:
	@echo "Проверка открытых портов..."
	@netstat -tlnp | grep 33333 || echo "Порт 33333 не занят"

# Просмотр логов
logs:
	@if [ -f "$(LOGDIR)/server.log" ]; then \
		tail -f $(LOGDIR)/server.log; \
	else \
		echo "Лог-файл не найден"; \
	fi

# Быстрое просмотр последних логов
log-tail:
	@if [ -f "$(LOGDIR)/server.log" ]; then \
		tail -20 $(LOGDIR)/server.log; \
	else \
		echo "Лог-файл не найден"; \
	fi

# Полный тест: сервер + клиент
test: test-server
	@echo "Сервер запущен. Теперь в другом терминале выполните:"
	@echo "./client_int16_t -H SHA1 -S c"
	@echo ""
	@echo "Для остановки сервера выполните: make stop-test"

# Перезапуск сервера
restart: stop-test test-server
	@echo "Сервер перезапущен"

.PHONY: all run run-port clean distclean install-deps create-config debug tree \
        test-server stop-test check-port logs log-tail test restart
