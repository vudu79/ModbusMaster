//
// Created by Андрей Водолацкий on 08.09.2025.
//


#ifndef MODBUSCOMMANDBUILDER_H
#define MODBUSCOMMANDBUILDER_H

#include <QObject>
#include <QByteArray>
#include <QString>

class ModbusCommandBuilder : public QObject
{
    Q_OBJECT
public:
    explicit ModbusCommandBuilder(QObject *parent = nullptr);

    Q_INVOKABLE QString buildCommand(int slaveId, int functionCode, int startAddress, int quantity, int value);

private:
    void appendByte(QByteArray &cmd, quint8 b);
    quint16 crc16(const QByteArray &data);
};

#endif // MODBUSCOMMANDBUILDER_HUILDER_H