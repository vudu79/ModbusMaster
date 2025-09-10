#include "ModbusCommandWidget.h"
#include "ModbusCommandWrighter.h"

#include <QApplication>       // Для создания и управления приложением Qt Widgets
#include <QWidget>            // Базовый класс для всех виджетов
#include <QLabel>             // Виджет для отображения текста
#include <QSpinBox>           // Виджет для ввода чисел с кнопками увеличения/уменьшения
#include <QComboBox>          // Виджет выпадающего списка
#include <QPushButton>        // Кнопка
#include <QVBoxLayout>        // Вертикальный менеджер компоновки
#include <QTextEdit>          // Многострочное текстовое поле


ModbusCommandWidget::ModbusCommandWidget(QWidget *parent) : QWidget(parent) {
    // выделяем память для буфера команды
    cmd = new QByteArray();
    // Создаем виджеты интерфейса
    // Метка и выпадающий список для выбора функции Modbus
    QLabel *functionLabel = new QLabel("Function Code:", this);
    functionCombo = new QComboBox(this);
    // Добавляем варианты функций с текстом и значениями
    functionCombo->addItem("Read Coils (0x01)", 1);
    functionCombo->addItem("Read Discrete Inputs (0x02)", 2);
    functionCombo->addItem("Read Holding Registers (0x03)", 3);
    functionCombo->addItem("Read Input Registers (0x04)", 4);
    functionCombo->addItem("Write Single Coil (0x05)", 5);
    functionCombo->addItem("Write Single Register (0x06)", 6);
    functionCombo->addItem("Write Multiple Coils (0x0F)", 15);
    functionCombo->addItem("Write Multiple Registers (0x10)", 16);

    // Метка и SpinBox для Slave ID (адрес устройства)
    QLabel *slaveIdLabel = new QLabel("Slave ID:", this);
    slaveIdSpin = new QSpinBox(this);
    slaveIdSpin->setRange(1, 247); // Диапазон адресов Modbus
    slaveIdSpin->setValue(1);

    // Метка и SpinBox для начального адреса
    QLabel *startAddressLabel = new QLabel("Start Address:", this);
    startAddressSpin = new QSpinBox(this);
    startAddressSpin->setRange(0, 65535);
    startAddressSpin->setValue(0);

    // Метка и SpinBox для количества регистров/бит
    QLabel *quantityLabel = new QLabel("Quantity:", this);
    quantitySpin = new QSpinBox(this);
    quantitySpin->setRange(1, 125); // Обычно ограничение Modbus
    quantitySpin->setValue(1);

    // Метка и SpinBox для значения (используется для записи)
    QLabel *valueLabel = new QLabel("Value:", this);
    valueSpin = new QSpinBox(this);
    valueSpin->setRange(0, 65535);
    valueSpin->setValue(0);

    // Кнопка для построения команды
    QPushButton *buildButton = new QPushButton("Build Command", this);

    // Кнопка для отправки команды
    sendButton = new QPushButton("Send Command", this);
    connect(sendButton, &QPushButton::clicked, this, &ModbusCommandWidget::handleSendButtonClicked);

    // Текстовое поле для вывода результата
    QLabel *inputLabel = new QLabel("Запрос:", this);
    inputText = new QTextEdit(this);
    inputText->setReadOnly(true); // Только для чтения

    // Текстовое поле для вывода результата
    QLabel *outputLabel = new QLabel("Ответ: ", this);
    outputText = new QTextEdit(this);
    outputText->setReadOnly(true); // Только для чтения

    // Компоновка элементов интерфейса

    // Вертикальный основной лэйаут
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Горизонтальные лэйауты для каждой строки с меткой и полем ввода
    QHBoxLayout *functionLayout = new QHBoxLayout();
    functionLayout->addWidget(functionLabel);
    functionLayout->addWidget(functionCombo);

    QHBoxLayout *slaveIdLayout = new QHBoxLayout();
    slaveIdLayout->addWidget(slaveIdLabel);
    slaveIdLayout->addWidget(slaveIdSpin);

    QHBoxLayout *startAddressLayout = new QHBoxLayout();
    startAddressLayout->addWidget(startAddressLabel);
    startAddressLayout->addWidget(startAddressSpin);

    QHBoxLayout *quantityLayout = new QHBoxLayout();
    quantityLayout->addWidget(quantityLabel);
    quantityLayout->addWidget(quantitySpin);

    QHBoxLayout *valueLayout = new QHBoxLayout();
    valueLayout->addWidget(valueLabel);
    valueLayout->addWidget(valueSpin);

    QVBoxLayout *inputLayout = new QVBoxLayout();
    inputLayout->addWidget(inputLabel);
    inputLayout->addWidget(inputText);

    QVBoxLayout *outputLayout = new QVBoxLayout();
    outputLayout->addWidget(outputLabel);
    outputLayout->addWidget(outputText);

    // Добавляем все лэйауты в основной вертикальный лэйаут
    mainLayout->addLayout(functionLayout);
    mainLayout->addLayout(slaveIdLayout);
    mainLayout->addLayout(startAddressLayout);
    mainLayout->addLayout(quantityLayout);
    mainLayout->addLayout(valueLayout);
    mainLayout->addWidget(buildButton);
    mainLayout->addLayout(inputLayout);
    mainLayout->addWidget(sendButton);
    mainLayout->addLayout(outputLayout);

    // Устанавливаем основной лэйаут для окна
    setLayout(mainLayout);

    // Подключаем сигнал изменения функции к слоту обновления видимости полей
    connect(functionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ModbusCommandWidget::updateVisibility);

    // Подключаем кнопку к слоту построения команды
    connect(buildButton, &QPushButton::clicked, this, &ModbusCommandWidget::buildCommand);

    // Инициализируем видимость полей в зависимости от выбранной функции
    updateVisibility(1);
}


void ModbusCommandWidget::onDataReceived(std::vector<uint8_t> data){
    QString hexString;
    for (uint8_t byte : data) {
        hexString += QString::number(byte, 16).rightJustified(2, '0');
    }
    outputText->setPlainText(hexString);
}

void ModbusCommandWidget::handleSendButtonClicked() {
    emit dataReady(*cmd);
}

quint16 ModbusCommandWidget::crc16(QByteArray &data) {
    quint16 crc = 0xFFFF;
    for (auto b: data) {
        crc ^= static_cast<quint8>(b);
        for (int i = 0; i < 8; i++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

// Слот для обновления видимости полей Quantity и Value в зависимости от функции
void ModbusCommandWidget::updateVisibility(int index) const {
    int func = functionCombo->currentData().toInt();
    inputText->setPlainText("Команда: " + QString::number(func));

    // int func = functionCombo->currentData().toInt();
    // Для функций чтения показываем Quantity, скрываем Value
    if (func == 1 || func == 2 || func == 3 || func == 4 || func == 15 || func == 16) {
        quantitySpin->setEnabled(true);
        // quantitySpin->parentWidget()->setVisible(true);
        valueSpin->setEnabled(false);
        // valueSpin->parentWidget()->setVisible(false);
    }
    // Для функций записи показываем Value, скрываем Quantity
    else if (func == 5 || func == 6) {
        quantitySpin->setEnabled(false);
        // quantitySpin->parentWidget()->setVisible(false);
        valueSpin->setEnabled(true);
        // valueSpin->parentWidget()->setVisible(true);
    } else {
        // По умолчанию показываем оба
        quantitySpin->setEnabled(true);
        // quantitySpin->parentWidget()->setVisible(true);
        valueSpin->setEnabled(true);
        // valueSpin->parentWidget()->setVisible(true);
    }
}

// Слот для построения Modbus команды в виде HEX строки
void ModbusCommandWidget::buildCommand() const {
    if (cmd->length()> 0) cmd->clear();
    int slaveId = slaveIdSpin->value();
    int func = functionCombo->currentData().toInt();
    int startAddr = startAddressSpin->value();
    int quantity = quantitySpin->value();
    int value = valueSpin->value();

    // Формируем команду по протоколу Modbus RTU (без CRC для простоты)

    cmd->append(static_cast<char>(slaveId)); // Адрес ведомого
    cmd->append(static_cast<char>(func)); // Код функции
    cmd->append(static_cast<char>((startAddr >> 8) & 0xFF)); // Старший байт адреса
    cmd->append(static_cast<char>(startAddr & 0xFF)); // Младший байт адреса

    // В зависимости от функции добавляем параметры
    switch (func) {
        case 1: // Read Coils
        case 2: // Read Discrete Inputs
        case 3: // Read Holding Registers
        case 4: // Read Input Registers
            cmd->append(static_cast<char>((quantity >> 8) & 0xFF)); // Старший байт количества
            cmd->append(static_cast<char>(quantity & 0xFF)); // Младший байт количества
            break;

        case 5: // Write Single Coil
            if (value == 0)
                cmd->append(char(0x00)).append(char(0x00)); // Выключить катушку
            else
                cmd->append(char(0xFF)).append(char(0x00)); // Включить катушку
            break;

        case 6: // Write Single Register
            cmd->append(static_cast<char>((value >> 8) & 0xFF)); // Старший байт значения
            cmd->append(static_cast<char>(value & 0xFF)); // Младший байт значения
            break;

        case 15: // Write Multiple Coils
        case 16: // Write Multiple Registers
            cmd->append(static_cast<char>((quantity >> 8) & 0xFF)); // Старший байт количества
            cmd->append(static_cast<char>(quantity & 0xFF)); // Младший байт количества
            // Для простоты не добавляем данные (payload)
            break;

        default: {
        }
    }

    // В конце - CRC
    quint16 crc = crc16(*cmd);
    cmd->append(static_cast<char>(crc & 0xFF));
    cmd->append(static_cast<char>(crc >> 8 & 0xFF));

    // Преобразуем команду в HEX строку с пробелами и заглавными буквами
    QString hexStr = cmd->toHex(' ').toUpper();

    // Выводим результат в текстовое поле
    inputText->setPlainText(hexStr);
}
