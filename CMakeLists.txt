cmake_minimum_required(VERSION 4.0)
project(ModbusMaster)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(Qt6_DIR "/Users/andrejvodolackij/Qt/6.9.2/macos/lib/cmake/Qt6")
set(CMAKE_PREFIX_PATH "/Users/andrejvodolackij/Qt/6.9.2/macos/lib/cmake")#If this line is not present, please add it
set(QML2_IMPORT_PATH "/Users/andrejvodolackij/Qt/6.9.2/macos/qml")

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)
qt_standard_project_setup()


add_executable(ModbusMaster main.cpp
        main.cpp
        main.cpp
        serial_port_util.h
        serial_port_util.cpp
        ModbusCommandWrighter.h
        ModbusCommandWrighter.cpp
        ModbusCommandWidget.cpp
        ModbusCommandWidget.h
        SerialThread.h)
target_link_libraries(ModbusMaster
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)
qt6_add_resources(RESOURCES qml.qrc)
target_sources(ModbusMaster PRIVATE ${RESOURCES})
