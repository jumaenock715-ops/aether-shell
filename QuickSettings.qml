import QtQuick

Item {
    id: root
    signal closeRequested()

    MouseArea { anchors.fill: parent; onClicked: root.closeRequested() }

    Rectangle {
        id: panel
        width: Math.min(390, root.width - 24); height: Math.min(col.implicitHeight + 40, root.height - Theme.barHeight - 24)
        x: root.width - width - 12
        y: Theme.barHeight + 8
        radius: 26
        color: Theme.launcherGlass; border.color: Theme.border
        MouseArea { anchors.fill: parent }

        Column {
            id: col
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 20 }
            spacing: 18
            Text { text: "Quick settings"; color: Theme.text; font.pixelSize: 18; font.bold: true }

            Grid {
                columns: 2; spacing: 12; width: parent.width
                property real cw: (width - spacing) / 2
                QuickTile { width: parent.cw; label: "Wi-Fi"; subtitle: System.wifi ? "Connected" : "Off"; on: System.wifi; onClicked: System.wifi = !System.wifi }
                QuickTile { width: parent.cw; label: "Bluetooth"; subtitle: System.bluetooth ? "On" : "Off"; on: System.bluetooth; onClicked: System.bluetooth = !System.bluetooth }
                QuickTile { width: parent.cw; label: "Dark mode"; subtitle: Theme.dark ? "On" : "Off"; on: Theme.dark; onClicked: SessionSettings.setDarkMode(!Theme.dark) }
                QuickTile { width: parent.cw; label: "Do not disturb"; subtitle: SessionSettings.doNotDisturb ? "On" : "Off"; on: SessionSettings.doNotDisturb; onClicked: SessionSettings.setDoNotDisturb(!SessionSettings.doNotDisturb) }
            }

            Column {
                width: parent.width; spacing: 6
                Text { text: "Display brightness (shell dimming)"; color: Theme.textDim; font.pixelSize: 13 }
                AetherSlider { width: parent.width; value: System.brightness; onMoved: (v) => System.brightness = v }
            }

            Column {
                width: parent.width; spacing: 6
                Row {
                    width: parent.width
                    Text { text: "Output volume"; color: Theme.textDim; font.pixelSize: 12 }
                    Item { width: 1; height: 1 }
                    Text { text: Audio.muted ? "Muted" : Audio.volume + "%"; color: Theme.textDim; font.pixelSize: 12 }
                }
                AetherSlider { width: parent.width; value: Audio.volume / 100; onMoved: (v) => Audio.setVolume(Math.round(v * 100)) }
                Row {
                    spacing: 14
                    Text {
                        text: Audio.muted ? "Unmute output" : "Mute output"
                        color: Theme.accent2; font.pixelSize: 12
                        MouseArea { anchors.fill: parent; onClicked: Audio.setMuted(!Audio.muted) }
                    }
                    Text {
                        text: "Refresh devices"; color: Theme.accent2; font.pixelSize: 12
                        MouseArea { anchors.fill: parent; onClicked: Audio.refresh() }
                    }
                }
                Text { text: "Microphone " + Audio.inputVolume + (Audio.inputMuted ? "% (muted)" : "%"); color: Theme.textDim; font.pixelSize: 11 }
                AetherSlider { width: parent.width; value: Audio.inputVolume / 100; onMoved: (v) => Audio.setInputVolume(Math.round(v * 100)) }
                Text {
                    text: Audio.inputMuted ? "Unmute microphone" : "Mute microphone"
                    color: Theme.accent2; font.pixelSize: 12
                    MouseArea { anchors.fill: parent; onClicked: Audio.setInputMuted(!Audio.inputMuted) }
                }
            }

            Column {
                width: parent.width; spacing: 5
                Row {
                    width: parent.width; spacing: 8
                    Text { text: "Clipboard history"; color: Theme.textDim; font.pixelSize: 12 }
                    Text {
                        text: "Clear"; color: Theme.accent2; font.pixelSize: 11
                        MouseArea { anchors.fill: parent; onClicked: Clipboard.clearHistory() }
                    }
                }
                ListView {
                    width: parent.width; height: Math.min(96, contentHeight)
                    clip: true; spacing: 3; model: Clipboard
                    delegate: Rectangle {
                        id: clipboardEntry
                        required property string text
                        required property int index
                        width: ListView.view.width; height: 30; radius: 10; color: Theme.surface
                        Text {
                            anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                            verticalAlignment: Text.AlignVCenter
                            text: clipboardEntry.text; color: Theme.text; font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                        MouseArea { anchors.fill: parent; onClicked: Clipboard.restore(clipboardEntry.index) }
                    }
                }
                Text { visible: Clipboard.count === 0; text: "No copied text yet"; color: Theme.textDim; font.pixelSize: 10 }
            }

            Text {
                width: parent.width; wrapMode: Text.WordWrap
                text: "Wi-Fi, Bluetooth and brightness are still demonstration controls. Audio and clipboard use session services."
                color: Theme.textDim; font.pixelSize: 10
            }
        }
    }
}
