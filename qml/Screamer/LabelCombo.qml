import QtQuick 2.1
import QtQuick.Controls 1.0

Item {
    id: labelCombo
    width: parent.width
    height: 48

    property real implicitComboWidth: 100
    property alias labelText: lbl.text

    property alias combo: combo

    Label {
        id: lbl
        anchors { right: combo.left; verticalCenter: parent.verticalCenter }
    }

    ComboBox {
        id: combo
        anchors { right: parent.right; verticalCenter: parent.verticalCenter }
        width: Math.min(implicitComboWidth, labelCombo.width - lbl.implicitWidth - 2)
        height: parent.height
    }

}
