import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0

import Screamer 1.0

Tab {
    title: "Program"

    property Settings settings

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        Rectangle {
            id: settingsPane
            width: 200
            Layout.minimumWidth: 150
            property real comboHeight: 22

            Column {
                anchors.fill: parent
                spacing: 5

                Item { width: parent.width; height: 30 }

                Button {
                    text: "Download"
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Item { width: parent.width; height: 30 }

                LabelCombo {
                    id: comboPort
                    labelText: "Port |"
                    height: settingsPane.comboHeight
                }

                LabelCombo {
                    id: comboBaud
                    labelText: "Baude Rate |"
                    height: settingsPane.comboHeight
                }

                LabelCombo {
                    id: comboChip
                    labelText: "Chip |"
                    height: settingsPane.comboHeight
                }

                LabelCombo {
                    id: comboReset
                    labelText: "Reset Type |"
                    height: settingsPane.comboHeight
                    property list<QtObject> mdl: [
                        ComboElement { text: "RTS"; value: settings.RTS },
                        ComboElement { text: "DTR"; value: settings.DTR }
                    ]
                    combo.model: mdl

                    combo.onCurrentIndexChanged: {
                        console.log("ResetCombo:", mdl[combo.currentIndex].text)
                        settings.resetType = mdl[combo.currentIndex].value
                    }
                }

                Item { width: parent.width; height: 30 }

                CheckBox {
                    id: autoOpenTerminal
                    text: "Open Terminal"
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                CheckBox {
                    id: logDownload
                    text: "Log Download"
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }


        }

        SplitView {
            orientation: Qt.Vertical
            Layout.fillWidth: true

            Rectangle {
                id: logPane
                color: "blue"
                opacity: 0.2
                Layout.minimumHeight: 150
                Layout.fillHeight: true
            }

            Rectangle {
                id: statusPane
                color: "red"
                opacity: 0.2
                Layout.minimumHeight: 150
            }
        }
    }

}
