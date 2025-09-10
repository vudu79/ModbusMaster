//
// Created by Андрей Водолацкий on 08.09.2025.
//

#ifndef UNTITLED_MODBUSCOMMANDWIDGET_H
#define UNTITLED_MODBUSCOMMANDWIDGET_H


#include <QApplication>       // Для создания и управления приложением Qt Widgets
#include <QWidget>            // Базовый класс для всех виджетов
#include <QLabel>             // Виджет для отображения текста
#include <QSpinBox>           // Виджет для ввода чисел с кнопками увеличения/уменьшения
#include <QComboBox>          // Виджет выпадающего списка
#include <QPushButton>        // Кнопка
#include <QVBoxLayout>        // Вертикальный менеджер компоновки
#include <QHBoxLayout>        // Горизонтальный менеджер компоновки
#include <QTextEdit>          // Многострочное текстовое поле

// Класс главного окна приложения
class ModbusCommandWidget : public QWidget {
    Q_OBJECT

public:
    explicit ModbusCommandWidget(QWidget *parent = nullptr);

signals:
    void dataReady(const QByteArray &data);

public slots:
    // Слот для обновления видимости полей Quantity и Value в зависимости от функции
    void updateVisibility(int func) const;

    // Слот для построения Modbus команды в виде HEX строки
    void buildCommand() const;

    // Слот - слушатель нажатия на кнопку sendFrame
    void handleSendButtonClicked();

    void onDataReceived(std::vector<uint8_t> data);

public slots:



public:
    QPushButton *sendButton;

private:
    static quint16 crc16( QByteArray &data);
    QByteArray *cmd; // Массив для формирования команды
    QComboBox *functionCombo;
    QSpinBox *slaveIdSpin;
    QSpinBox *startAddressSpin;
    QSpinBox *quantitySpin;
    QSpinBox *valueSpin;
    QTextEdit *outputText;
    QTextEdit *inputText;
};

#endif //UNTITLED_MODBUSCOMMANDWIDGET_H
