
#include <unistd.h>
#include <termios.h>
#include "ModbusCommandWidget.h"
#include "ModbusCommandWrighter.h"


// В терминале запускаем для виртуальных порта и соединяем их
// ➜  ~ socat -d -d pty,raw,echo=0 pty,raw,echo=0


// UI Qt QWidget
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    ModbusCommandWidget window;

    ModbusMasterProcessor* master = new ModbusMasterProcessor("/dev/ttys001", B9600);


    QObject::connect(&window, &ModbusCommandWidget::dataReady, master, &ModbusMasterProcessor::sendFrame);
    QObject::connect(master, &ModbusMasterProcessor::dataReceived, &window, &ModbusCommandWidget::onDataReceived);
    window.setWindowTitle("Modbus Master Emulator");
    window.resize(400, 300);
    window.show();
    // master->readLoop();


   // ThreadSafeQueue<std::vector<uint8_t>> *outQueue = new ThreadSafeQueue<std::vector<uint8_t>>();
   //  ThreadSafeQueue<std::vector<uint8_t>> *inQueue = new ThreadSafeQueue<std::vector<uint8_t>>();
   //  SerialThread *serialThread = new SerialThread("/dev/ttys001", *outQueue, *inQueue);
   //  serialThread->start();
   //  while (1) {
   //      std::vector<u_int8_t> data = {'e','r','t','7','g'};
   //      outQueue->enqueue(data);
   //      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
   //
   //  }
    return app.exec();

}
















