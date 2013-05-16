import QtQuick 2.1
import QtQuick.Controls 1.0

Item {
    id: labelCombo
    width: parent.width
    height: 48

    property real implicitComboWidth: 100
    property alias labelText: lbl.text

    property int value: 0

    property alias combo: comboBox

    Label {
        id: lbl
        anchors { right: combo.left; verticalCenter: parent.verticalCenter }
    }

    ComboBox {
        id: comboBox
        anchors { right: parent.right; verticalCenter: parent.verticalCenter }
        width: Math.min(implicitComboWidth, labelCombo.width - lbl.implicitWidth - 2)
        height: parent.height
    }

    onValueChanged: {
        console.log("Value Changed to", value)
        for (var i=0; i<combo.model.count; ++i) {
            if (combo.model.get(i).value == value) {
                combo.currentIndex = i;
                return;
            }
        }
    }

}
