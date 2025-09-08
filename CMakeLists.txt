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
find_package(Qt6 REQUIRED COMPONENTS Qml)
find_package(Qt6 COMPONENTS Core Quick Qml REQUIRED)
qt_standard_project_setup()


add_executable(ModbusMaster main.cpp
        main.cpp
        main.cpp
        serial_port_util.h
        serial_port_util.cpp
        modbus_rtu_wrighter.h
        modbus_rtu_wrighter.cpp
        ModbusCommandWidget.cpp
        ModbusCommandWidget.h
        ModbusCommandBuilder.h
        ModbusCommandBuilder.cpp)
target_link_libraries(ModbusMaster
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Qml
)
qt6_add_resources(RESOURCES qml.qrc)
target_sources(ModbusMaster PRIVATE ${RESOURCES})
