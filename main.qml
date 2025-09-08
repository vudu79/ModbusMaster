import QtQuick 2.15
import QtQuick.Controls 2.15
import Modbus 1.0

ApplicationWindow {
    property int functionCode: 1

    height: 400
    title: qsTr("Modbus Command Builder (QML + C++)")
    visible: true
    width: 400

    ModbusCommandBuilder {
        id: builder

    }
    Column {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Row {
            spacing: 10

            Label {
                text: "Function Code:"
            }
            ComboBox {
                id: functionCombo

                currentIndex: 0
                model: [
                    {
                        text: "Read Coils (0x01)",
                        value: 1
                    },
                    {
                        text: "Read Discrete Inputs (0x02)",
                        value: 2
                    },
                    {
                        text: "Read Holding Registers (0x03)",
                        value: 3
                    },
                    {
                        text: "Read Input Registers (0x04)",
                        value: 4
                    },
                    {
                        text: "Write Single Coil (0x05)",
                        value: 5
                    },
                    {
                        text: "Write Single Register (0x06)",
                        value: 6
                    },
                    {
                        text: "Write Multiple Coils (0x0F)",
                        value: 15
                    },
                    {
                        text: "Write Multiple Registers (0x10)",
                        value: 16
                    }
                ]
                width: 250

                onCurrentIndexChanged: {
                    functionCode = model[currentIndex].value;
                }
            }
        }
        Row {
            spacing: 10

            Label {
                text: "Slave ID:"
            }
            SpinBox {
                id: slaveIdSpin

                from: 1
                to: 247
                value: 1
                width: 80
            }
        }
        Row {
            spacing: 10

            Label {
                text: "Start Address:"
            }
            SpinBox {
                id: startAddressSpin

                from: 0
                to: 65535
                value: 0
                width: 80
            }
        }
        Row {
            id: quantityRow

            spacing: 10
            visible: functionCode === 1 || functionCode === 2 || functionCode === 3 || functionCode === 4 || functionCode === 15 || functionCode === 16

            Label {
                text: "Quantity:"
            }
            SpinBox {
                id: quantitySpin

                enabled: visible
                from: 1
                to: 125
                value: 1
                width: 80
            }
        }
        Row {
            id: valueRow

            spacing: 10
            visible: functionCode === 5 || functionCode === 6

            Label {
                text: "Value:"
            }
            SpinBox {
                id: valueSpin

                enabled: visible
                from: 0
                to: 65535
                value: 0
                width: 80
            }
        }
        Button {
            text: "Build Command"

            onClicked: {
                var hexCmd = builder.buildCommand(slaveIdSpin.value, functionCode, startAddressSpin.value, quantitySpin.value, valueSpin.value);
                outputText.text = hexCmd;
            }
        }
        TextArea {
            id: outputText

            font.family: "monospace"
            height: 150
            readOnly: true
            text: ""
            wrapMode: TextArea.Wrap
        }
    }
}