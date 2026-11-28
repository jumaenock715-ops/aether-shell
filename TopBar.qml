import QtQuick

Rectangle {
    id: root
    signal launcherRequested()
    signal quickSettingsRequested()
    signal notificationsRequested()

    height: Theme.barHeight
    color: Theme.panel

    property string clock: Qt.formatDateTime(new Date(), "ddd d MMM   hh:mm")
    Timer {
        interval: 1000; running: true; repeat: true
        onTriggered: root.clock = Qt.formatDateTime(new Date(), "ddd d MMM   hh:mm")
    }

    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }

    // Aether wordmark: launcher access lives in the centred bottom taskbar.
    Text {
        anchors.left: parent.left; anchors.leftMargin: 18
        anchors.verticalCenter: parent.verticalCenter
        text: "AETHER"
        color: Theme.text; font.pixelSize: 11; font.bold: true; font.letterSpacing: 2
    }

    // Centre: clock (opens notifications)
    Text {
        anchors.centerIn: parent
        text: root.clock
        color: Theme.text; font.pixelSize: 13
        MouseArea {
            anchors.fill: parent; anchors.margins: -8
            cursorShape: Qt.PointingHandCursor
            onClicked: root.notificationsRequested()
        }
    }

    // Right: status area (opens quick settings)
    Rectangle {
        anchors.right: parent.right; anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        width: statusRow.width + 20; height: 24; radius: 12
        color: statusArea.containsMouse ? Theme.surface : "transparent"

        Row {
            id: statusRow
            anchors.centerIn: parent
            spacing: 12
            Text { text: "Wi-Fi";  color: System.wifi ? Theme.text : Theme.textDim; font.pixelSize: 12 }
            Text { text: "BT";     color: System.bluetooth ? Theme.text : Theme.textDim; font.pixelSize: 12 }
            Text { text: System.battery + "%"; color: Theme.text; font.pixelSize: 12 }
            Rectangle {
                visible: Notifications.count > 0
                width: 16; height: 16; radius: 8; color: Theme.accent
                Text { anchors.centerIn: parent; text: Notifications.count; color: "white"; font.pixelSize: 10; font.bold: true }
            }
        }
        MouseArea {
            id: statusArea; anchors.fill: parent; hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.quickSettingsRequested()
        }
    }
}
