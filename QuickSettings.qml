import QtQuick

Item {
    id: root
    signal closeRequested()

    MouseArea { anchors.fill: parent; onClicked: root.closeRequested() }

    Rectangle {
        id: panel
        width: 340; height: col.height + 36
        x: root.width - width - 12
        y: Theme.barHeight + 8
        radius: 20
        color: Theme.panel; border.color: Theme.border

        MouseArea { anchors.fill: parent }   // swallow clicks

        Column {
            id: col
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 18 }
            spacing: 14

            Text { text: "Quick settings"; color: Theme.text; font.pixelSize: 15; font.bold: true }

            Grid {
                columns: 2; spacing: 10
                width: parent.width
                property real cw: (width - spacing) / 2
                QuickTile { width: parent.cw; label: "Wi-Fi";       subtitle: System.wifi ? "Connected" : "Off"; on: System.wifi;      onClicked: System.wifi = !System.wifi }
                QuickTile { width: parent.cw; label: "Bluetooth";   subtitle: System.bluetooth ? "On" : "Off"; on: System.bluetooth; onClicked: System.bluetooth = !System.bluetooth }
                QuickTile { width: parent.cw; label: "Dark mode";   subtitle: Theme.dark ? "On" : "Off";       on: Theme.dark;        onClicked: SessionSettings.setDarkMode(!Theme.dark) }
                QuickTile { width: parent.cw; label: "Do not disturb"; subtitle: System.dnd ? "On" : "Off";   on: System.dnd;        onClicked: System.dnd = !System.dnd }
            }

            Column {
                width: parent.width; spacing: 6
                Text { text: "Display brightness"; color: Theme.textDim; font.pixelSize: 12 }
                AetherSlider {
                    width: parent.width
                    value: System.brightness
                    onMoved: (v) => System.brightness = v
                }
            }

            Text {
                width: parent.width; wrapMode: Text.WordWrap
                text: "Interface demonstration. Controls are not yet connected to hardware services."
                color: Theme.textDim; font.pixelSize: 11
            }
        }
    }
}
