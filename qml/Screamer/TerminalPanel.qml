import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0

import Screamer 1.0

Tab {
    id: tab
    title: "Terminal"

    property Settings settings: null

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        Terminal {
            id: terminal
            settings: tab.settings
        }

        Rectangle {
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
                    text: "Clear"
                    anchors.horizontalCenter: parent.horizontalCenter
                    onClicked: terminal.text = ""
                }

                Button {
                    text: "Reset"
                    anchors.horizontalCenter: parent.horizontalCenter
                    onClicked: programmer.resetMicro(settings)
                }

                Item { width: parent.width; height: 30 }

                CheckBox {
                    id: enableTerminal
                    text: "Enable"
                    anchors.horizontalCenter: parent.horizontalCenter
                    checked: false

                    property bool value: terminal.active
                    onValueChanged: checked = value
                    onCheckedChanged: terminal.active = checked
                }

                CheckBox {
                    id: echo
                    text: "Echo"
                    anchors.horizontalCenter: parent.horizontalCenter
                    property bool value: settings.echo
                    onValueChanged: checked = value
                    onCheckedChanged: settings.echo = checked
                }

                CheckBox {
                    id: wrap
                    text: "Wrap"
                    anchors.horizontalCenter: parent.horizontalCenter
                    checked: true
                }

                Item { width: parent.width; height: 30 }

                LabelCombo {
                    id: comboBaud
                    labelText: "Baude Rate |"
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

                    value: settings.baudTerminal
                    combo.onCurrentIndexChanged: {
                        settings.baudTerminal = baudModel.get(combo.currentIndex).value
                    }
                }

                LabelCombo {
                    id: comboDataBits
                    labelText: "Data Bits |"
                    height: settingsPane.comboHeight
                    implicitComboWidth: settingsPane.comboWidth
                    combo.model: ListModel {
                        id: dataBitsModel
                        ListElement { text: "8"; value: Serial.Data8 }
                        ListElement { text: "7"; value: Serial.Data7 }
                    }

                    value: settings.dataBits
                    combo.onCurrentIndexChanged: {
                        settings.dataBits = dataBitsModel.get(combo.currentIndex).value
                    }
                }

                LabelCombo {
                    id: comboStopBits
                    labelText: "Stop Bits |"
                    height: settingsPane.comboHeight
                    implicitComboWidth: settingsPane.comboWidth
                    combo.model: ListModel {
                        id: stopBitsModel
                        ListElement { text: "1"; value: Serial.OneStop }
                        ListElement { text: "1.5"; value: Serial.OneAndHalfStop }
                        ListElement { text: "2"; value: Serial.TwoStop }
                    }

                    value: settings.stopBits

                    combo.onCurrentIndexChanged: {
                        settings.stopBits = stopBitsModel.get(combo.currentIndex).value
                    }
                }

                LabelCombo {
                    id: comboParity
                    labelText: "Stop Bits |"
                    height: settingsPane.comboHeight
                    implicitComboWidth: settingsPane.comboWidth
                    combo.model: ListModel {
                        id: parityModel
                        ListElement { text: "Odd"; value: Serial.OddParity }
                        ListElement { text: "Even"; value: Serial.EvenParity }
                        ListElement { text: "None"; value: Serial.NoParity }
                    }

                    value: settings.parity

                    combo.onCurrentIndexChanged: {
                        settings.parity = parityModel.get(combo.currentIndex).value
                    }
                }

                GroupBox {
                    title: "Display (No Effect ATM)"
                    Column {
                        ExclusiveGroup { id: displayGroup }
                        RadioButton {
                            text: "ASCII"
                            exclusiveGroup: displayGroup

                            property bool value: settings.terminalCharacters == Settings.Ascii

                            onValueChanged: checked = value
                            onCheckedChanged: {
                                if (checked == true)
                                    settings.terminalCharacters = Settings.Ascii
                            }
                        }
                        RadioButton {
                            text: "Hex"
                            exclusiveGroup: displayGroup

                            property bool value: settings.terminalCharacters == Settings.Hex

                            onValueChanged: checked = value
                            onCheckedChanged: {
                                if (checked == true)
                                    settings.terminalCharacters = Settings.Hex
                            }
                        }
                        RadioButton {
                            text: "Dec"
                            exclusiveGroup: displayGroup

                            property bool value: settings.terminalCharacters == Settings.Dec

                            onValueChanged: checked = value
                            onCheckedChanged: {
                                if (checked == true)
                                    settings.terminalCharacters = Settings.Dec
                            }
                        }
                    }
                }

                Item { width: parent.width; height: 30 }
            }


        }

        Rectangle {
            Layout.fillWidth: true

            TextArea {
                id: terminalView
                anchors.fill: parent
                wrapMode: wrap.checked ? Text.Wrap : Text.NoWrap

                text: terminal.text

                onTextChanged: {
                    cursorPosition = text.length
                }
            }

        }
    }

}
