#include <QPushButton>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <termios.h>
#include <sys/select.h>

#include "modbus_rtu_wrighter.h"
#include "serial_port_util.h"

#define BUF_SIZE 256


// В терминале запускаем для виртуальных порта и соединяем их
// ➜  ~ socat -d -d pty,raw,echo=0 pty,raw,echo=0

int main(int argc, char *argv[]) {
    QGuiApplication application(argc, argv);
    QQmlApplicationEngine engine;

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    try {
        const ModbusRTUWrighter writer("/dev/ttys001", B9600);
        writer.wrightLoop();
    }
    catch (const std::exception& ex) {
        std::cerr << "Ошибка: " << ex.what() << std::endl;
        return 1;
    }


 // Открываем порты
    // int fd1 = open("/dev/ttys000", O_RDWR | O_NOCTTY);
    // int fd2 = open("/dev/ttys002", O_RDWR | O_NOCTTY);
    // if (fd2 < 0 ) {
    //     perror("open");
    //     return 1;
    // }
    // if (configure_port(fd2) < 0 ) {
    //     return 1;
    // }
    //
    // while (1) {
    //     uint8_t slave_addr;
    //     uint16_t start_reg, reg_count;
    //
    //     printf("Введите адрес устройства (slave) (0-247): ");
    //     if (scanf("%hhu", &slave_addr) != 1) break;
    //     if (slave_addr == 0 || slave_addr > 247) {
    //         printf("Неверный адрес устройства\n");
    //         continue;
    //     }
    //
    //     printf("Введите начальный адрес регистра (0-65535): ");
    //     if (scanf("%hu", &start_reg) != 1) break;
    //
    //     printf("Введите количество регистров для чтения (1-125): ");
    //     if (scanf("%hu", &reg_count) != 1) break;
    //     if (reg_count == 0 || reg_count > 125) {
    //         printf("Неверное количество регистров\n");
    //         continue;
    //     }
    //
    //     // Очистка stdin после scanf
    //     int c; while ((c = getchar()) != '\n' && c != EOF);
    //
    //     // Формируем пакет Modbus RTU
    //     uint8_t packet[8];
    //     packet[0] = slave_addr;
    //     packet[1] = 0x03; // функция 3
    //     packet[2] = (start_reg >> 8) & 0xFF; // старший байт адреса регистра
    //     packet[3] = start_reg & 0xFF;        // младший байт адреса регистра
    //     packet[4] = (reg_count >> 8) & 0xFF; // старший байт количества регистров
    //     packet[5] = reg_count & 0xFF;        // младший байт количества регистров
    //
    //     uint16_t crc = crc16_modbus(packet, 6);
    //     packet[6] = crc & 0xFF;       // CRC low byte
    //     packet[7] = (crc >> 8) & 0xFF; // CRC high byte
    //
    //     printf("Сформирован пакет Modbus RTU:\n");
    //     printf("Адрес устройства: 0x%02X\n", packet[0]);
    //     printf("Функция: 0x%02X (Чтение регистров)\n", packet[1]);
    //     printf("Начальный адрес регистра: 0x%04X\n", start_reg);
    //     printf("Количество регистров: %d\n", reg_count);
    //     printf("CRC16: 0x%04X\n", crc);
    //     print_buffer("Пакет для отправки:", packet, 8);
    //
    //     // Отправляем пакет в порт 1
    //     ssize_t w = write(fd2, packet, 8);
    //     if (w != 8) {
    //         perror("write");
    //         break;
    //     }
    //     printf("Пакет отправлен в порт 1\n");

        // Читаем ответ из порта 2
        // Максимальный размер ответа: slave(1) + func(1) + byte count(1) + 2*reg_count + CRC(2)
        // size_t max_response_len = 5 + 2 * reg_count;
        // uint8_t response[256];
        // ssize_t r = read(fd2, response, max_response_len);
        // if (r < 0) {
        //     perror("read");
        //     break;
        // } else if (r == 0) {
        //     printf("Нет данных в порту 2\n");
        // } else {
        //     print_buffer("Получен ответ из порта 2:", response, r);
        // }
        //
        // printf("\n");
    // }

    // close(fd2);
    // // close(fd2);
    // return 0;








    //
    // int fd1 = open("/dev/ttys000", O_RDWR | O_NOCTTY);
    // int fd2 = open("/dev/ttys001", O_RDWR | O_NOCTTY);
    //
    // if (fd1 < 0 || fd2 < 0) {
    //     perror("open");
    //     return 1;
    // }
    //
    // if (configure_port(fd1) < 0 || configure_port(fd2) < 0) {
    //     return 1;
    // }
    //
    // char input_buf[BUF_SIZE];
    //
    // while (1) {
    //     // Читаем строку с клавиатуры
    //     printf("Введите строку для отправки в порт 1: ");
    //     if (fgets(input_buf, sizeof(input_buf), stdin) == NULL) {
    //         printf("Ошибка чтения или EOF. Выход.\n");
    //         break;
    //     }
    //
    //     // Удаляем символ перевода строки
    //     size_t len = strlen(input_buf);
    //     if (len > 0 && input_buf[len - 1] == '\n') {
    //         input_buf[len - 1] = '\0';
    //         len--;
    //     }
    //
    //     // Отправляем в порт 1
    //     ssize_t written = write(fd1, input_buf, len);
    //     if (written < 0) {
    //         perror("write to fd1");
    //         break;
    //     }
    //
    //     printf("Отправлено в порт 1: %s\n", input_buf);
    //
    //     // Читаем из порта 2
    //     ssize_t n = read(fd2, input_buf, sizeof(input_buf));
    //     if (n < 0) {
    //         perror("read from fd2");
    //         break;
    //     } else if (n == 0) {
    //         printf("Порт 2 закрыт или нет данных.\n");
    //     } else {
    //         // Выводим полученные данные
    //         print_buffer("Данные из порта 2:", input_buf, n);
    //     }
    // }
    //
    // close(fd1);
    // close(fd2);
    // return 0;







    // int fd1 = open("/dev/ttys000", O_RDWR | O_NOCTTY);
    // // int fd2 = open("/dev/ttys002", O_RDWR | O_NOCTTY);
    //
    // if (fd1 < 0) {
    //     perror("open");
    //     return 1;
    // }
    //
    // if (configure_port(fd1) < 0) {
    //     return 1;
    // }
    //
    // while (1) {
    //     char buf[256];
    //     ssize_t n;
    //
    //     n = read(fd1, buf, sizeof(buf));
    //     if (n > 0) {
    //         print_buffer("fd1 -> fd2:", buf, n);
    //         // write(fd2, buf, n);
    //     }
    // }
    //
    // close(fd1);
    // return 0;
    // return QGuiApplication::exec();
}
