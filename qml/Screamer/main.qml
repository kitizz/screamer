import QtQuick 2.1
import QtQuick.Controls 1.0
import Screamer 1.0

ApplicationWindow {
    width: 900
    height: 600

    Settings {
        id: settings
    }

    TabView {
        id: tabView
        anchors.fill: parent
        ProgrammerPanel { settings: settings }
        TerminalPanel { settings: settings }
    }
}
