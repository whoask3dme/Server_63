# Компилятор и флаги
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
LIBS = -lssl -lcrypto -lpthread -lboost_program_options

# Директории
SRCDIR = src
OBJDIR = obj
BINDIR = bin
CONFIGDIR = config
LOGDIR = log
TESTDIR = tests

# Исходные файлы
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
# Исключаем main.cpp из тестов
TEST_SOURCES = $(filter-out $(SRCDIR)/main.cpp, $(SOURCES))
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
TEST_OBJECTS = $(TEST_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
TARGET = $(BINDIR)/server
TEST_TARGET = $(BINDIR)/test_runner

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

$(TESTDIR):
	mkdir -p $(TESTDIR)

# Дополнительные цели
all: $(TARGET)

# Запуск сервера с параметрами по умолчанию
run: all $(LOGDIR)
	./$(TARGET)

# Запуск с конкретным портом
run-port: all $(LOGDIR)
	./$(TARGET) -p 44444

# Очистка
clean:
	rm -rf $(OBJDIR) $(BINDIR)

distclean: clean
	rm -rf $(LOGDIR)/*

# Установка зависимостей (для Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install -y libssl-dev build-essential libboost-program-options-dev libunittest++-dev

# Создание конфигурационного файла
create-config:
	@echo "Создание файла конфигурации..."
	@mkdir -p $(CONFIGDIR)
	@echo "# Файл базы пользователей сервера" > $(CONFIGDIR)/scale.conf
	@echo "# Формат: логин:пароль" >> $(CONFIGDIR)/scale.conf
	@echo "" >> $(CONFIGDIR)/scale.conf
	@echo "user:P@ssWord" >> $(CONFIGDIR)/scale.conf
	@echo "testuser:testpass123" >> $(CONFIGDIR)/scale.conf
	@echo "admin:admin123" >> $(CONFIGDIR)/scale.conf
	@echo "user1:secret123" >> $(CONFIGDIR)/scale.conf
	@echo "ibst128:SuperSecretPassword" >> $(CONFIGDIR)/scale.conf
	@echo "Файл конфигурации создан: $(CONFIGDIR)/scale.conf"

# Отладочная сборка
debug: CXXFLAGS += -DDEBUG -Og
debug: clean all

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
	./$(TARGET) -p 33333 &
	@sleep 2

# Остановка тестового сервера
stop-test:
	@pkill -f "$(TARGET)" || true
	@sleep 1
	@echo "Сервер остановлен"

# Проверка порта
check-port:
	@echo "Проверка открытых портов..."
	@netstat -tlnp | grep :33333 || echo "Порт 33333 не занят"

# Просмотр логов
logs:
	@if [ -f "$(LOGDIR)/server.log" ]; then \
		tail -f $(LOGDIR)/server.log; \
	else \
		echo "Лог-файл не найден"; \
	fi

# Быстрый просмотр последних логов
log-tail:
	@if [ -f "$(LOGDIR)/server.log" ]; then \
		tail -20 $(LOGDIR)/server.log; \
	else \
		echo "Лог-файл не найден"; \
	fi

# Интеграционный тест: сервер + клиент
integration-test: all $(LOGDIR)
	@if netstat -tln 2>/dev/null | grep -q :33333; then \
		echo "Порт 33333 занят, останавливаем предыдущий сервер..."; \
		make stop-test; \
		sleep 1; \
	fi
	@echo "Запуск сервера для тестирования на порту 33333..."
	./$(TARGET) -p 33333 &
	@sleep 2
	@echo "Сервер запущен. Теперь в другом терминале выполните:"
	@echo "./client_int16_t -H SHA1 -S c"
	@echo ""
	@echo "Для остановки сервера выполните: make stop-test"

# Перезапуск сервера
restart: stop-test test-server
	@echo "Сервер перезапущен"

# Информация о сборке
info:
	@echo "Компилятор: $(CXX)"
	@echo "Флаги: $(CXXFLAGS)"
	@echo "Библиотеки: $(LIBS)"
	@echo "Исходные файлы: $(SOURCES)"
	@echo "Объектные файлы: $(OBJECTS)"

# Модульное тестирование
unit-test: $(TEST_TARGET) | $(TESTDIR)
	@echo "Запуск модульных тестов..."
	./$(TEST_TARGET)

# Исправленная цель для тестов - исключаем main.o
$(TEST_TARGET): $(TEST_OBJECTS) $(TESTDIR)/test_runner.cpp $(wildcard $(TESTDIR)/*.cpp) | $(BINDIR) $(TESTDIR)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) $(TESTDIR)/test_runner.cpp $(wildcard $(TESTDIR)/*.cpp) $(TEST_OBJECTS) -o $(TEST_TARGET) $(LIBS) -lUnitTest++

# Создание тестового runner
$(TESTDIR)/test_runner.cpp: | $(TESTDIR)
	@echo "// Auto-generated test runner" > $@
	@echo "#define UNITTEST_NO_LIBRARY" >> $@
	@echo "#include <UnitTest++/UnitTest++.h>" >> $@
	@echo "" >> $@
	@echo "int main() {" >> $@
	@echo "    return UnitTest::RunAllTests();" >> $@
	@echo "}" >> $@

# Полное тестирование
test-all: unit-test integration-test
	@echo "Все тесты завершены"

# Создание структуры тестов - УПРОЩЕННАЯ ВЕРСИЯ
create-tests: | $(TESTDIR)
	@echo "Создание структуры тестов..."
	@if [ ! -f "$(TESTDIR)/test_authentication.cpp" ]; then \
		echo "// Тесты для AuthenticationManager" > $(TESTDIR)/test_authentication.cpp; \
		echo "#include <UnitTest++/UnitTest++.h>" >> $(TESTDIR)/test_authentication.cpp; \
		echo "// Проверка создания объектов" >> $(TESTDIR)/test_authentication.cpp; \
		echo "" >> $(TESTDIR)/test_authentication.cpp; \
		echo "SUITE(AuthenticationManagerTest) {" >> $(TESTDIR)/test_authentication.cpp; \
		echo "    TEST(Placeholder) {" >> $(TESTDIR)/test_authentication.cpp; \
		echo "        // Временный тест" >> $(TESTDIR)/test_authentication.cpp; \
		echo "        CHECK(true);" >> $(TESTDIR)/test_authentication.cpp; \
		echo "    }" >> $(TESTDIR)/test_authentication.cpp; \
		echo "}" >> $(TESTDIR)/test_authentication.cpp; \
		echo "Файл test_authentication.cpp создан"; \
	else \
		echo "Файл test_authentication.cpp уже существует"; \
	fi
	@if [ ! -f "$(TESTDIR)/test_dataprocessor.cpp" ]; then \
		echo "// Тесты для DataProcessor" > $(TESTDIR)/test_dataprocessor.cpp; \
		echo "#include <UnitTest++/UnitTest++.h>" >> $(TESTDIR)/test_dataprocessor.cpp; \
		echo "// Проверка создания объектов" >> $(TESTDIR)/test_dataprocessor.cpp; \
		echo "" >> $(TESTDIR)/test_dataprocessor.cpp; \
		echo "SUITE(DataProcessorTest) {" >> $(TESTDIR)/test_dataprocessor.cpp; \
		echo "    TEST(Placeholder) {" >> $(TESTDIR)/test_dataprocessor.cpp; \
		echo "        // Временный тест" >> $(TESTDIR)/test_dataprocessor.cpp; \
		echo "        CHECK(true);" >> $(TESTDIR)/test_dataprocessor.cpp; \
		echo "    }" >> $(TESTDIR)/test_dataprocessor.cpp; \
		echo "}" >> $(TESTDIR)/test_dataprocessor.cpp; \
		echo "Файл test_dataprocessor.cpp создан"; \
	else \
		echo "Файл test_dataprocessor.cpp уже существует"; \
	fi
	@if [ ! -f "$(TESTDIR)/test_logger.cpp" ]; then \
		echo "// Тесты для Logger" > $(TESTDIR)/test_logger.cpp; \
		echo "#include <UnitTest++/UnitTest++.h>" >> $(TESTDIR)/test_logger.cpp; \
		echo "// Проверка создания объектов" >> $(TESTDIR)/test_logger.cpp; \
		echo "" >> $(TESTDIR)/test_logger.cpp; \
		echo "SUITE(LoggerTest) {" >> $(TESTDIR)/test_logger.cpp; \
		echo "    TEST(Placeholder) {" >> $(TESTDIR)/test_logger.cpp; \
		echo "        // Временный тест" >> $(TESTDIR)/test_logger.cpp; \
		echo "        CHECK(true);" >> $(TESTDIR)/test_logger.cpp; \
		echo "    }" >> $(TESTDIR)/test_logger.cpp; \
		echo "}" >> $(TESTDIR)/test_logger.cpp; \
		echo "Файл test_logger.cpp создан"; \
	else \
		echo "Файл test_logger.cpp уже существует"; \
	fi
	@if [ ! -f "$(TESTDIR)/test_clientsession.cpp" ]; then \
		echo "// Тесты для ClientSession" > $(TESTDIR)/test_clientsession.cpp; \
		echo "#include <UnitTest++/UnitTest++.h>" >> $(TESTDIR)/test_clientsession.cpp; \
		echo "// Проверка создания объектов" >> $(TESTDIR)/test_clientsession.cpp; \
		echo "" >> $(TESTDIR)/test_clientsession.cpp; \
		echo "SUITE(ClientSessionTest) {" >> $(TESTDIR)/test_clientsession.cpp; \
		echo "    TEST(Placeholder) {" >> $(TESTDIR)/test_clientsession.cpp; \
		echo "        // Временный тест" >> $(TESTDIR)/test_clientsession.cpp; \
		echo "        CHECK(true);" >> $(TESTDIR)/test_clientsession.cpp; \
		echo "    }" >> $(TESTDIR)/test_clientsession.cpp; \
		echo "}" >> $(TESTDIR)/test_clientsession.cpp; \
		echo "Файл test_clientsession.cpp создан"; \
	else \
		echo "Файл test_clientsession.cpp уже существует"; \
	fi
	@if [ ! -f "$(TESTDIR)/test_server.cpp" ]; then \
		echo "// Тесты для Server" > $(TESTDIR)/test_server.cpp; \
		echo "#include <UnitTest++/UnitTest++.h>" >> $(TESTDIR)/test_server.cpp; \
		echo "// Проверка создания объектов" >> $(TESTDIR)/test_server.cpp; \
		echo "" >> $(TESTDIR)/test_server.cpp; \
		echo "SUITE(ServerTest) {" >> $(TESTDIR)/test_server.cpp; \
		echo "    TEST(Placeholder) {" >> $(TESTDIR)/test_server.cpp; \
		echo "        // Временный тест" >> $(TESTDIR)/test_server.cpp; \
		echo "        CHECK(true);" >> $(TESTDIR)/test_server.cpp; \
		echo "    }" >> $(TESTDIR)/test_server.cpp; \
		echo "}" >> $(TESTDIR)/test_server.cpp; \
		echo "Файл test_server.cpp создан"; \
	else \
		echo "Файл test_server.cpp уже существует"; \
	fi

.PHONY: all run run-port clean distclean install-deps create-config debug tree \
        test-server stop-test check-port logs log-tail integration-test restart info \
        unit-test test-all create-tests
