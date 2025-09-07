//
// Created by Андрей Водолацкий on 07.09.2025.
//

#ifndef MODBUS_RTU_READER_H
#define MODBUS_RTU_READER_H

#include <string>
#include <vector>
#include <cstdint>
#include <termios.h> // Для speed_t

// Функция вычисления CRC16 Modbus
uint16_t crc16_modbus(const uint8_t* data, size_t length);

// Класс для чтения и разбора Modbus RTU пакетов
class ModbusRTUWrighter {
public:
    // Конструктор: device - имя порта, baudRate - скорость передачи
    ModbusRTUWrighter(const std::string& device, speed_t baudRate);

    // Деструктор: закрывает порт
    ~ModbusRTUWrighter();

    // Запуск цикла чтения и разбора пакетов
    void wrightLoop() const;

private:
    int fd; // Дескриптор порта

    // Настройка параметров порта
    void configurePort(speed_t baudRate) const;

    // Обработка завершённого пакета
    static void createPackets(const std::vector<uint8_t>& packet);
};

#endif // MODBUS_RTU_READER_H