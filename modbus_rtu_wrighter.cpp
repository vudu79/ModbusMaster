#include "modbus_rtu_wrighter.h"

#include <iostream>     // Для вывода в консоль
#include <vector>       // Для динамического массива байт (буфера)
#include <chrono>       // Для измерения времени (таймаут)
#include <fcntl.h>      // Для работы с файловыми дескрипторами (open)
#include <unistd.h>     // Для read, close, usleep
#include <termios.h>    // Для настройки последовательного порта
#include <cstring>      // Для strerror
#include <stdexcept>    // Для исключений

// Функция вычисления CRC16 Modbus
uint16_t crc16_modbus(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF; // Инициализация CRC стартовым значением
    for (size_t pos = 0; pos < length; pos++) {
        crc ^= (uint16_t) data[pos]; // XOR с очередным байтом
        for (int i = 8; i != 0; i--) {
            // Обработка каждого бита
            if ((crc & 0x0001) != 0) {
                // Если младший бит равен 1
                crc >>= 1; // Сдвиг вправо
                crc ^= 0xA001; // XOR с полиномом Modbus
            } else {
                crc >>= 1; // Иначе просто сдвиг вправо
            }
        }
    }
    return crc; // Возвращаем вычисленное CRC
}

// Класс для чтения и разбора Modbus RTU пакетов
ModbusRTUWrighter::ModbusRTUWrighter(const std::string &device, speed_t baudRate) : fd(-1)
// Инициализируем дескриптор невалидным значением
{
    fd = open(device.c_str(), O_RDWR | O_NOCTTY | O_SYNC); // Открываем порт
    if (fd < 0) {
        // Если не удалось открыть, выбрасываем исключение с сообщением
        throw std::runtime_error("Не удалось открыть порт " + device + ": " + strerror(errno));
    }
    configurePort(baudRate); // Настраиваем параметры порта
}

// Деструктор: закрывает порт при уничтожении объекта
ModbusRTUWrighter::~ModbusRTUWrighter() {
    if (fd >= 0) close(fd);
}

// Основной цикл чтения данных из порта
void ModbusRTUWrighter::wrightLoop() const {
    std::vector<uint8_t> buffer; // Буфер для накопления байт пакета
    using clock = std::chrono::steady_clock; // Тип часов для измерения времени
    auto lastByteTime = clock::now(); // Время получения последнего байта

    // Таймаут 3.5 символа в микросекундах (пример для 9600 бод)
    // 1 символ ~ 11 бит / 9600 бод = ~1.15 мс
    // 3.5 символа ~ 4 мс = 4000 мкс
    const auto timeout = std::chrono::microseconds(4000);

    while (true) {
        uint8_t byte;
        ssize_t n = read(fd, &byte, 1); // Читаем 1 байт из порта
        if (n < 0) {
            std::cerr << "Ошибка чтения: " << strerror(errno) << std::endl;
            continue; // При ошибке чтения продолжаем попытки
        } else if (n == 0) {
            // Нет данных, можно подождать немного
            usleep(1000); // 1 мс
            continue;
        }

        auto now = clock::now(); // Текущее время

        if (!buffer.empty()) {
            // Если буфер не пуст, проверяем паузу между байтами
            auto diff = std::chrono::duration_cast<std::chrono::microseconds>(now - lastByteTime);
            if (diff > timeout) {
                // Если пауза больше 3.5 символов — считаем пакет завершённым
                createPackets(buffer); // Обрабатываем накопленный пакет
                buffer.clear(); // Очищаем буфер для нового пакета
            }
        }

        buffer.push_back(byte); // Добавляем байт в буфер
        lastByteTime = now; // Обновляем время последнего байта
    }
}

// Настройка параметров последовательного порта
void ModbusRTUWrighter::configurePort(speed_t baudRate) const {
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        close(fd);
        throw std::runtime_error("Ошибка tcgetattr: " + std::string(strerror(errno)));
    }

    cfsetospeed(&tty, baudRate); // Установка скорости передачи (выход)
    cfsetispeed(&tty, baudRate); // Установка скорости передачи (вход)

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8 бит данных
    tty.c_iflag &= ~IGNBRK; // Не игнорировать BREAK
    tty.c_lflag = 0; // Режим "raw" (без обработки)
    tty.c_oflag = 0; // Вывод без обработки
    tty.c_cc[VMIN] = 0; // Минимум байт для чтения
    tty.c_cc[VTIME] = 0; // Таймаут чтения

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Отключаем управление потоком XON/XOFF
    tty.c_cflag |= (CLOCAL | CREAD); // Включаем прием и локальный режим
    tty.c_cflag &= ~(PARENB | PARODD); // Без проверки четности
    tty.c_cflag &= ~CSTOPB; // 1 стоп-бит
    tty.c_cflag &= ~CRTSCTS; // Отключаем аппаратное управление потоком

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        close(fd);
        throw std::runtime_error("Ошибка tcsetattr: " + std::string(strerror(errno)));
    }
}

// Обработка завершённого пакета
void ModbusRTUWrighter::createPackets(const std::vector<uint8_t> &packet) {
    if (packet.size() < 4) {
        // Минимальный размер пакета: адрес(1) + функция(1) + CRC(2)
        std::cerr << "Пакет слишком короткий, игнорируем" << std::endl;
        return;
    }

    // Вычисляем CRC по всем байтам, кроме последних двух (CRC в конце)
    uint16_t crcCalc = crc16_modbus(packet.data(), packet.size() - 2);
    // Извлекаем CRC из пакета (младший байт + старший байт)
    uint16_t crcPacket = packet[packet.size() - 2] | (packet[packet.size() - 1] << 8);

    if (crcCalc != crcPacket) {
        std::cerr << "Ошибка CRC, пакет отброшен" << std::endl;
        return;
    }

    // Если CRC верен — выводим пакет в консоль
    std::cout << "Получен корректный пакет Modbus RTU: ";
    for (auto b: packet) {
        printf("%02X ", b);
    }

    std::cout << std::endl;

    // Здесь можно добавить обработку данных пакета по протоколу Modbus
}

