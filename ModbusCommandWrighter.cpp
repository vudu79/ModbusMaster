#include "ModbusCommandWrighter.h"
#include "serial_port_util.h"
#include "SerialThread.h"


#include <iostream>     // Для вывода в консоль
#include <vector>       // Для динамического массива байт (буфера)
#include <chrono>       // Для измерения времени (таймаут)
#include <fcntl.h>      // Для работы с файловыми дескрипторами (open)
#include <unistd.h>     // Для read, close, usleep
#include <termios.h>    // Для настройки последовательного порта
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
ModbusMasterProcessor::ModbusMasterProcessor(const char *device, speed_t baudRate, QWidget *parent) : QWidget(parent) {
    outQueue = new ThreadSafeQueue<std::vector<uint8_t> >();
    inQueue = new ThreadSafeQueue<std::vector<uint8_t> >();
    serialThread = new SerialThread(device, *outQueue, *inQueue);
    serialThread->start();
    read_thread = std::thread(&ModbusMasterProcessor::readLoop, this);


    // this->fd = -1;
    // fd = open(device.c_str(), O_RDWR | O_NOCTTY | O_SYNC); // Открываем порт
    // if (fd < 0) {
    //     // Если не удалось открыть, выбрасываем исключение с сообщением
    //     throw std::runtime_error("Не удалось открыть порт " + device + ": " + strerror(errno));
    // }
    // configurePort(baudRate); // Настраиваем параметры порта
}

// Деструктор: закрывает порт при уничтожении объекта
ModbusMasterProcessor::~ModbusMasterProcessor() {
    // if (fd >= 0) ::close(fd);
    serialThread->stop();
    if (read_thread.joinable()) read_thread.join();
}

// Основной цикл для формирования пакета и отправки в порт (консоль)
void ModbusMasterProcessor::wrightLoop() const {
    // uint8_t packet[8]; // Буфер для  пакета
    //
    // while (true) {
    //     createPacket(packet);
    //     print_buffer("Пакет для отправки:", packet, 8);
    //
    //     // Отправляем пакет в порт 1
    //     ssize_t w = write(fd, packet, 8);
    //
    //     if (w != 8) {
    //         perror("write");
    //         break;
    //     }
    //     printf("Пакет размером %ld отправлен в порт 1\n", w);
    // }
}

// Настройка параметров последовательного порта (консоль)
void ModbusMasterProcessor::configurePort(speed_t baudRate) const {
    // struct termios tty;
    // if (tcgetattr(fd, &tty) != 0) {
    //     ::close(fd);
    //     throw std::runtime_error("Ошибка tcgetattr: " + std::string(strerror(errno)));
    // }
    //
    // cfsetospeed(&tty, baudRate); // Установка скорости передачи (выход)
    // cfsetispeed(&tty, baudRate); // Установка скорости передачи (вход)
    //
    // tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8 бит данных
    // tty.c_iflag &= ~IGNBRK; // Не игнорировать BREAK
    // tty.c_lflag = 0; // Режим "raw" (без обработки)
    // tty.c_oflag = 0; // Вывод без обработки
    // tty.c_cc[VMIN] = 0; // Минимум байт для чтения
    // tty.c_cc[VTIME] = 0; // Таймаут чтения
    //
    // tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Отключаем управление потоком XON/XOFF
    // tty.c_cflag |= (CLOCAL | CREAD); // Включаем прием и локальный режим
    // tty.c_cflag &= ~(PARENB | PARODD); // Без проверки четности
    // tty.c_cflag &= ~CSTOPB; // 1 стоп-бит
    // tty.c_cflag &= ~CRTSCTS; // Отключаем аппаратное управление потоком
    //
    // if (tcsetattr(fd, TCSANOW, &tty) != 0) {
    //     ::close(fd);
    //     throw std::runtime_error("Ошибка tcsetattr: " + std::string(strerror(errno)));
    // }
}

// Обработка завершённого пакета (консоль)
void ModbusMasterProcessor::createPacket(uint8_t *packet) {
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
        int c;
        while ((c = getchar()) != '\n' && c != EOF);

        // Формируем пакет Modbus RTU
        packet[0] = slave_addr;
        packet[1] = 0x03; // функция 3
        packet[2] = (start_reg >> 8) & 0xFF; // старший байт адреса регистра
        packet[3] = start_reg & 0xFF; // младший байт адреса регистра
        packet[4] = (reg_count >> 8) & 0xFF; // старший байт количества регистров
        packet[5] = reg_count & 0xFF; // младший байт количества регистров

        uint16_t crc = crc16_modbus(packet, 6);
        packet[6] = crc & 0xFF; // CRC low byte
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

// Отправляем пакет в порт (консоль)
// void ModbusMasterProcessor::sendFrame(const QByteArray &data) {
//     qDebug() << "Receiver got data:" << data.toHex();
//     printf("нажата кнопка");
//
//     ssize_t w = write(fd, data.constData(), data.length());
//
//     if (w != data.length()) {
//         perror("write");
//     }
//     delete(&data);
//     printf("Пакет размером %ld отправлен в порт 1\n", w);
// }

// Отправляем пакет в порт (консоль)
void ModbusMasterProcessor::sendFrame(const QByteArray &data) const {
    std::vector<uint8_t> vector(data.begin(), data.end());
    outQueue->enqueue(vector);
}


void ModbusMasterProcessor::readLoop() {
    std::vector<uint8_t> data;
    while (1) {
        while (inQueue->try_dequeue(data)) {
            checkRequestADU(data);
        }
    }
}


// Основной цикл чтения данных из порта (консоль)
// void ModbusMasterProcessor::readLoop() {
// std::vector<uint8_t> buffer; // Буфер для накопления байт пакета
// buffer.clear();
// using clock = std::chrono::steady_clock; // Тип часов для измерения времени
// auto lastByteTime = clock::now(); // Время получения последнего байта
//
// // Таймаут 3.5 (по протоколу) символа в микросекундах (используем 9600 бод)
// // 1 символ ~ 11 бит / 9600 бод = ~1.15 мс
// // 3.5 символа ~ 4 мс = 4000 мкс
// constexpr auto timeout = std::chrono::microseconds(4000);
//
// while (true) {
//     uint8_t byte;
//     ssize_t n = read(fd, &byte, 1); // Читаем 1 байт из порта
//
//     if (n < 0) {
//         std::cerr << "Ошибка чтения: " << strerror(errno) << std::endl;
//         continue; // При ошибке чтения продолжаем попытки
//     }
//
//     if (n == 0 && buffer.size() != 8) {
//         // Нет данных, ждем
//         usleep(1000); // 1 мс
//         continue;
//     }
//
//     auto now = clock::now(); // Текущее время
//
//     if (!buffer.empty()) {
//         // Если буфер не пуст, проверяем паузу между байтами
//         auto diff = std::chrono::duration_cast<std::chrono::microseconds>(now - lastByteTime);
//         if (diff > timeout) {
//             // Если пауза больше 3.5 символов — считаем пакет завершённым
//             checkRequestADU(buffer); // Обрабатываем накопленный пакет
//             buffer.clear(); // Очищаем буфер для нового пакета
//         }
//     }
//     // Добавляем байт в буфер
//     if (n == 1) {
//         buffer.push_back(byte);
//         lastByteTime = now; // Обновляем время последнего байта
//     }
// }
// }

// Обработка завершённого пакета
void ModbusMasterProcessor::checkRequestADU(std::vector<uint8_t> &packet) {
    // Проверяем минимальный размер (5 байт)
    if (packet.size() >= 5) {
        quint8 slaveAddress = static_cast<quint8>(packet.at(0));
        quint8 functionCode = static_cast<quint8>(packet.at(1));

        if (functionCode & 0x80) {
            // Ошибочный ответ
            quint8 exceptionCode = static_cast<quint8>(packet.at(2));
            qDebug() << "Ошибка от ведомого. Код исключения:" << QString::number(exceptionCode, 16);
        } else {
            // Нормальный ответ
        }
    } else {
        qDebug() << "Ответ короткий, возможно неверный или повреждён";
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
    std::cout << "Получен корректный пакет ADU: ";
    for (auto b: packet) {
        printf("%02X ", b);
    }
    std::cout << std::endl;

    emit dataReceived(packet);

    // QString str = QString::fromStdString(std::string(packet.begin(), packet.end()));
    // ui->textEdit->append(str);
}

void ModbusMasterProcessor::printHEXPacket(std::vector<uint8_t> &packet) const {
    uint8_t functionNumber = packet[1];
    uint8_t slaveNumber = packet[0];
    if (functionNumber & 0x80) {
        printf("Пакет с кодом ошибки на команду № %d от устройства № %d сформирован: ", functionNumber, slaveNumber);
    }
    printf("Ответный пакет на команду № %d от устройства № %d сформирован: ", functionNumber, slaveNumber);
    for (auto b: packet) {
        printf("%02X ", b);
    }
    std::cout << std::endl;
}
