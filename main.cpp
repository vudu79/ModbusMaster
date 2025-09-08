#include <QPushButton>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QWidget>            // Базовый класс для всех виджетов
#include <QApplication>       // Для создания и управления приложением Qt Widgets
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <termios.h>
#include <sys/select.h>

#include "ModbusCommandBuilder.h"
#include "ModbusCommandWidget.h"
#include "modbus_rtu_wrighter.h"
#include "serial_port_util.h"



// В терминале запускаем для виртуальных порта и соединяем их
// ➜  ~ socat -d -d pty,raw,echo=0 pty,raw,echo=0

// UI Qt QWidget
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    ModbusCommandWidget window;
    window.setWindowTitle("Modbus Command Builder (Qt Widgets)");
    window.resize(400, 300);
    window.show();

    return app.exec();
}

// UI QML

// int main(int argc, char *argv[])
// {
//     QGuiApplication app(argc, argv);
//
//     qmlRegisterType<ModbusCommandBuilder>("Modbus", 1, 0, "ModbusCommandBuilder");
//
//     QQmlApplicationEngine engine;
//     engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
//     if (engine.rootObjects().isEmpty())
//         return -1;
//
//     return app.exec();
// }



// int main(int argc, char *argv[]) {
//     QGuiApplication application(argc, argv);
//     QQmlApplicationEngine engine;
//
//     engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
//     if (engine.rootObjects().isEmpty()) {
//         return -1;
//     }
//     return QGuiApplication::exec();
//
//     try {
//         const ModbusRTUWrighter writer("/dev/ttys003", B9600);
//         writer.wrightLoop();
//     }
//     catch (const std::exception& ex) {
//         std::cerr << "Ошибка: " << ex.what() << std::endl;
//         return 1;
//     }
// }
