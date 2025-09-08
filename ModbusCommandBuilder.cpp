
#include "ModbusCommandBuilder.h"

ModbusCommandBuilder::ModbusCommandBuilder(QObject *parent) : QObject(parent)
{
}

void ModbusCommandBuilder::appendByte(QByteArray &cmd, quint8 b)
{
    cmd.append(b);
}

quint16 ModbusCommandBuilder::crc16(const QByteArray &data)
{
    quint16 crc = 0xFFFF;
    for (auto b : data) {
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

QString ModbusCommandBuilder::buildCommand(int slaveId, int functionCode, int startAddress, int quantity, int value)
{
    QByteArray cmd;
    appendByte(cmd, static_cast<quint8>(slaveId));
    appendByte(cmd, static_cast<quint8>(functionCode));
    appendByte(cmd, static_cast<quint8>((startAddress >> 8) & 0xFF));
    appendByte(cmd, static_cast<quint8>(startAddress & 0xFF));

    switch (functionCode) {
    case 1: case 2: case 3: case 4:
        appendByte(cmd, static_cast<quint8>((quantity >> 8) & 0xFF));
        appendByte(cmd, static_cast<quint8>(quantity & 0xFF));
        break;
    case 5:
        if (value == 0) {
            appendByte(cmd, 0x00);
            appendByte(cmd, 0x00);
        } else {
            appendByte(cmd, 0xFF);
            appendByte(cmd, 0x00);
        }
        break;
    case 6:
        appendByte(cmd, static_cast<quint8>((value >> 8) & 0xFF));
        appendByte(cmd, static_cast<quint8>(value & 0xFF));
        break;
    case 15: case 16:
        appendByte(cmd, static_cast<quint8>((quantity >> 8) & 0xFF));
        appendByte(cmd, static_cast<quint8>(quantity & 0xFF));
        // Для простоты не добавляем payload
        break;
    default:
        // Неизвестная функция - возвращаем пустую строку
        return QString();
    }

    // Добавляем CRC16
    quint16 crc = crc16(cmd);
    appendByte(cmd, static_cast<quint8>(crc & 0xFF));
    appendByte(cmd, static_cast<quint8>((crc >> 8) & 0xFF));

    // Формируем HEX строку с пробелами
    QString hexStr;
    for (int i = 0; i < cmd.size(); ++i) {
        hexStr += QString("%1").arg(static_cast<unsigned char>(cmd[i]), 2, 16, QLatin1Char('0')).toUpper();
        if (i != cmd.size() - 1)
            hexStr += " ";
    }

    return hexStr;
}