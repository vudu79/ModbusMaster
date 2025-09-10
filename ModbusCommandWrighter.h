//
// Created by Андрей Водолацкий on 07.09.2025.
//

#ifndef MODBUS_RTU_READER_H
#define MODBUS_RTU_READER_H

#include "ModbusCommandWidget.h"

#include <qobjectdefs.h>
#include <qtmetamacros.h>
#include <string>
#include <vector>
#include <termios.h> // Для speed_t

#include "SerialThread.h"

// Команды Modbus
// Чтение дискретных выходов (Coils)	0x01
// Чтение дискретных входов (Discrete Inputs)	0x02
// Чтение аналоговых выходов (Holding Registers)	0x03
// Чтение аналоговых входов (Input Registers)	0x04
// Запись одного дискретного выхода (Coil)	0x05
// Запись одного регистра (Holding Register)	0x06
// Запись нескольких дискретных выходов (Coils)	0x0F
// Запись нескольких регистров (Holding Registers)	0x10
#define READ_COILS 0x01
#define READ_DISCRETE_INPUTS 0x02
#define READ_HOLDING_REGISTERS 0x03
#define READ_INPUT_REGISTERS 0x04
#define WRITE_SINGLE_COIL 0x05
#define WRITE_SINGLE_REGISTER 0x06
#define WRITE_MULTIPLE_COILS 0x0F
#define WRITE_MULTIPLE_REGISTERS 0x10

// Коды ошибок
// 0x01	Неверная функция (Illegal Function)
// 0x02	Неверный адрес данных (Illegal Data Address)
// 0x03	Неверные данные (Illegal Data Value)
// 0x04	Ошибка устройства (Slave Device Failure)
// 0x05	Ошибка проверки (Acknowledge)
// 0x06	Ошибка устройства занято (Slave Device Busy)
// 0x08	Ошибка памяти (Memory Parity Error)
#define ILLEGAL_FUNCTION 0x01
#define ILLEGAL_DATA_ADDRESS 0x02
#define ILLEGAL_DATA_VALUE 0x03
#define SLAVE_DEVICE_FAILURE 0x04
#define ACKNOWLEDGE_FAILURE 0x05
#define SLAVE_DEVICE_BUSY_FAILURE 0x06
#define MEMORY_PARITY_FAILURE 0x07


// Функция вычисления CRC16 Modbus
uint16_t crc16_modbus(const uint8_t *data, size_t length);

// Класс для чтения и разбора Modbus RTU пакетов
class ModbusMasterProcessor : public QWidget {
    Q_OBJECT

public:
    // Конструктор: device - имя порта, baudRate - скорость передачи
    ModbusMasterProcessor(const char *device, speed_t baudRate, QWidget *parent = nullptr);

    // Деструктор: закрывает порт
    ~ModbusMasterProcessor() override;

    // Запуск цикла чтения и разбора пакетов (консоль)
    void wrightLoop() const;

    // Запуск цикла в котором проверяем очередь приема пакетов
    void readLoop() ;

private:

    ThreadSafeQueue<std::vector<uint8_t> > *outQueue;
    ThreadSafeQueue<std::vector<uint8_t> > *inQueue;
    SerialThread *serialThread;
    std::thread read_thread;

    // Настройка параметров порта
    void configurePort(speed_t baudRate) const;

    // Обработка завершённого пакета
    static void createPacket(uint8_t *packet);

signals:
    // сигнал который процессор отправляет по получению данных из очереди
    void dataReceived(std::vector<uint8_t> data);

public slots:
    void sendFrame(const QByteArray &data) const;

    void checkRequestADU(std::vector<uint8_t> &packet) ;

    void printHEXPacket(std::vector<uint8_t> &packet) const;
};

#endif // MODBUS_RTU_READER_H
