import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0

import Screamer 1.0

Tab {
    title: "Program"

    property Settings settings

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        Programmer {
            id: programmer
        }

        Item {
            id: settingsPane
            width: 250
            Layout.minimumWidth: 150
            property real comboHeight: 22
            property real comboWidth: 150

            Column {
                id: settingsColumn
                anchors.fill: parent
                spacing: 5

                Item { width: parent.width; height: 30 }

                Button {
                    text: programmer.isProgramming ? "Cancel" : "Download"
                    anchors.horizontalCenter: parent.horizontalCenter
                    onClicked: {
                        if (programmer.isProgramming) {
                            programmer.stopProgramming()
                        } else {
                            programmer.programMicro(settings)
                        }
                    }
                }

                Button {
                    text: "Reset"
                    anchors.horizontalCenter: parent.horizontalCenter
                    onClicked: programmer.resetMicro(settings)
                }

                Item { width: parent.width; height: 30 }

                LabelCombo {
                    id: comboPort
                    labelText: "Port |"
                    height: settingsPane.comboHeight
                    implicitComboWidth: settingsPane.comboWidth
                    combo.model: ListModel { id: portModel }

                    property var portList: settings.availablePorts
                    onPortListChanged: {
                        portModel.clear()
                        var newIndex = 0
                        for(var i=0; i<portList.length; ++i) {
                            portModel.append({ "text": portList[i]})
                            if (portList[i] == settings.portName)
                                newIndex = i
                        }
                        combo.currentIndex = newIndex
                        updatePort()
                    }
                    combo.onCurrentIndexChanged: updatePort()

                    function updatePort() {
                        if (portModel.count == 0)
                            settings.portName = "";
                        else
                            settings.portName = portModel.get(combo.currentIndex).text
                    }
                }

                LabelCombo {
                    id: comboBaud
                    labelText: "Baud Rate |"
                    height: settingsPane.comboHeight
                    implicitComboWidth: settingsPane.comboWidth
                    combo.model: ListModel {
                        id: baudModel
                        ListElement { text: "1200"; value: Serial.Baud1200 }
                        ListElement { text: "2400"; value: Serial.Baud2400 }
                        ListElement { text: "4800"; value: Serial.Baud4800 }
                        ListElement { text: "9600"; value: Serial.Baud9600 }
                        ListElement { text: "19200"; value: Serial.Baud19200 }
                        ListElement { text: "38400"; value: Serial.Baud38400 }
                        ListElement { text: "57600"; value: Serial.Baud57600 }
                        ListElement { text: "115200"; value: Serial.Baud115200 }
                    }

                    value: settings.baudProgram
                    combo.onCurrentIndexChanged: {
                        settings.baudProgram = baudModel.get(combo.currentIndex).value
                    }
                }

                LabelCombo {
                    id: comboChip
                    labelText: "Chip |"
                    height: settingsPane.comboHeight
                    implicitComboWidth: settingsPane.comboWidth
                    combo.model: ListModel {
                        id: chipModel
                        ListElement { text: "Atmega16"; value: Settings.Atmega168 }
                        ListElement { text: "Atmega32"; value: Settings.Atmega328 }
                        ListElement { text: "Atmega32u4"; value: Settings.Atmega32u4 }
                    }

                    value: settings.chip
                    combo.onCurrentIndexChanged: {
                        settings.chip = chipModel.get(combo.currentIndex).value
                    }
                }

                LabelCombo {
                    id: comboReset
                    labelText: "Reset Type |"
                    height: settingsPane.comboHeight
                    implicitComboWidth: settingsPane.comboWidth
                    combo.model: ListModel {
                        id: resetModel
                        ListElement { text: "RTS"; value: Settings.RTS }
                        ListElement { text: "DTR"; value: Settings.DTR }
                    }

                    value: settings.resetType

                    combo.onCurrentIndexChanged: {
                        settings.resetType = resetModel.get(combo.currentIndex).value
                    }
                }

                Item { width: parent.width; height: 30 }

                CheckBox {
                    id: autoOpenTerminal
                    text: "Open Terminal"
                    anchors.horizontalCenter: parent.horizontalCenter
                    property bool value: settings.autoOpenTerminal
                    onValueChanged: checked = value
                    onCheckedChanged: settings.autoOpenTerminal = checked
                }

                CheckBox {
                    id: logDownload
                    text: "Log Download"
                    anchors.horizontalCenter: parent.horizontalCenter
                    property bool value: settings.logDownload
                    onValueChanged: checked = value
                    onCheckedChanged: settings.logDownload = checked
                }
            }
        }

        SplitView {
            orientation: Qt.Vertical
            Layout.fillWidth: true

            Item {
                id: logPane
                Layout.minimumHeight: 150
                Layout.fillHeight: true

                TextArea {
                    id: terminalView
                    anchors.fill: parent
                    anchors.margins: 5
                    wrapMode: Text.Wrap

                    text: settings.log

                    onTextChanged: {
                        cursorPosition = text.length
                    }
                }
            }

            Item {
                id: statusPane
                Layout.minimumHeight: 150
                Column {
                    spacing: 5
                    anchors.fill: parent
                    anchors.margins: 5

                    Item { // Hex File
                        anchors { left: parent.left; right: parent.right }
                        height: childrenRect.height
    //                    height: fileText.height + 10

                        Label {
                            id: fileText
                            anchors { left: parent.left; right: btnBrowse.left; verticalCenter: btnBrowse.verticalCenter}
                            anchors.margins: 5

                            text: settings.hexFile
                            elide: Text.ElideMiddle

                        }
                        Button {
                            id: btnBrowse
                            anchors { right: parent.right}
                            anchors.margins: 5

                            text: "Browse..."
                            onClicked: hexFileDialog.open()
                        }

                        FileDialog {
                            id: hexFileDialog
                            title: "Choose a hex file..."
                            nameFilters: ["Hex Files (*.hex)", "All Files (*)"]
                            onAccepted: {
                                console.log("Chose:", hexFileDialog.fileUrls)
                                console.log(hexFileDialog.fileUrl)
                                settings.hexFile = hexFileDialog.fileUrl
                            }
                        }
                    }

                    Text { // Progress Text
                        property int currentAddress: programmer.currentAddress
                        property int totalAddress: programmer.lastAddress
                        text: {
                            if (totalAddress == 0) {
                                return "Buffer Empty"
                            } else {
                                return ("Loading address " + currentAddress + " of " + totalAddress)
                            }
                        }
                    }

                    ProgressBar {
                        anchors { left: parent.left; right: parent.right }
                        minimumValue: 0; maximumValue: 1
                        value: programmer.progress
                    }

                    Item {
                        anchors { left: parent.left; right: parent.right }
                        height: statusRow.height
                        Row {
                            id: statusRow
                            spacing: 10
                            Rectangle {
                                height: 20; width: height
                                color: {
                                    switch (programmer.status) {
                                    case Programmer.Idle: return "white"
                                    case Programmer.Connecting: return "yellow"
                                    case Programmer.Connected: return "green"
                                    case Programmer.Programming: return "blue"
                                    case Programmer.Failure: return "orange"
                                    case Programmer.Error: return "red"
                                    default: return "white"
                                    }
                                }
                            }
                            Text {
                                text: programmer.statusText
                            }
                        }
                    }

                    Text { text: "Retries: " + programmer.resends }

                }
            }
        }
    }

}
