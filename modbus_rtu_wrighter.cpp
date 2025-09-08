#include "modbus_rtu_wrighter.h"
#include "serial_port_util.h"

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
    uint8_t packet[8]; // Буфер для  пакета

    while (true) {
        createPacket(packet);
        print_buffer("Пакет для отправки:", packet, 8);

        // Отправляем пакет в порт 1
        ssize_t w = write(fd, packet, 8);
        if (w != 8) {
            perror("write");
            break;


        }
        printf("Пакет размером %ld отправлен в порт 1\n" , w);
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
void ModbusRTUWrighter::createPacket(uint8_t *packet) {
    while (1) {
        uint8_t slave_addr;
        uint16_t start_reg, reg_count;

        printf("Введите адрес устройства (slave) (0-247): ");
        if (scanf("%hhu", &slave_addr) != 1) break;
        if (slave_addr == 0 || slave_addr > 247) {
            printf("Неверный адрес устройства\n");
            continue;
        }

        printf("Введите начальный адрес регистра (0-65535): ");
        if (scanf("%hu", &start_reg) != 1) break;

        printf("Введите количество регистров для чтения (1-125): ");
        if (scanf("%hu", &reg_count) != 1) break;
        if (reg_count == 0 || reg_count > 125) {
            printf("Неверное количество регистров\n");
            continue;
        }

        // Очистка stdin после scanf
        int c; while ((c = getchar()) != '\n' && c != EOF);

        // Формируем пакет Modbus RTU
        packet[0] = slave_addr;
        packet[1] = 0x03; // функция 3
        packet[2] = (start_reg >> 8) & 0xFF; // старший байт адреса регистра
        packet[3] = start_reg & 0xFF;        // младший байт адреса регистра
        packet[4] = (reg_count >> 8) & 0xFF; // старший байт количества регистров
        packet[5] = reg_count & 0xFF;        // младший байт количества регистров

        uint16_t crc = crc16_modbus(packet, 6);
        packet[6] = crc & 0xFF;       // CRC low byte
        packet[7] = (crc >> 8) & 0xFF; // CRC high byte

        printf("Сформирован пакет Modbus RTU:\n");
        printf("Адрес устройства: 0x%02X\n", packet[0]);
        printf("Функция: 0x%02X (Чтение регистров)\n", packet[1]);
        printf("Начальный адрес регистра: 0x%04X\n", start_reg);
        printf("Количество регистров: %d\n", reg_count);
        printf("CRC16: 0x%04X\n", crc);
        // print_buffer("Пакет для отправки:", packet, 8);

        break;
    }
}

