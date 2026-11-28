import QtQuick

Item {
    id: root
    signal closeRequested()
    property string expandedTile: ""

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
                QuickTile { width: parent.cw; label: "Wi-Fi"; subtitle: System.wifi ? "Connected" : "Off"; on: System.wifi; expanded: root.expandedTile === "Wi-Fi"; onClicked: System.wifi = !System.wifi; onDetailsClicked: root.expandedTile = root.expandedTile === "Wi-Fi" ? "" : "Wi-Fi" }
                QuickTile { width: parent.cw; label: "Bluetooth"; subtitle: System.bluetooth ? "On" : "Off"; on: System.bluetooth; expanded: root.expandedTile === "Bluetooth"; onClicked: System.bluetooth = !System.bluetooth; onDetailsClicked: root.expandedTile = root.expandedTile === "Bluetooth" ? "" : "Bluetooth" }
                QuickTile { width: parent.cw; label: "Dark mode"; subtitle: Theme.dark ? "On" : "Off"; on: Theme.dark; expanded: root.expandedTile === "Dark mode"; onClicked: SessionSettings.setDarkMode(!Theme.dark); onDetailsClicked: root.expandedTile = root.expandedTile === "Dark mode" ? "" : "Dark mode" }
                QuickTile { width: parent.cw; label: "Do not disturb"; subtitle: SessionSettings.doNotDisturb ? "On" : "Off"; on: SessionSettings.doNotDisturb; expanded: root.expandedTile === "Do not disturb"; onClicked: SessionSettings.setDoNotDisturb(!SessionSettings.doNotDisturb); onDetailsClicked: root.expandedTile = root.expandedTile === "Do not disturb" ? "" : "Do not disturb" }
            }

            Rectangle {
                width: parent.width
                height: root.expandedTile.length > 0 ? 58 : 0
                visible: root.expandedTile.length > 0
                radius: 14
                color: Theme.surface
                border.color: Theme.border
                Column {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 3
                    Text { text: root.expandedTile; color: Theme.text; font.pixelSize: 12; font.bold: true }
                    Text {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        color: Theme.textDim
                        font.pixelSize: 10
                        text: root.expandedTile === "Wi-Fi" ? "Wireless networking is a demo toggle; scanning and access-point selection are not connected yet."
                            : root.expandedTile === "Bluetooth" ? "Bluetooth is a demo toggle. Device discovery and pairing will be added with the connectivity service."
                            : root.expandedTile === "Dark mode" ? "Changes the Aether shell theme and saves the preference."
                            : "Suppresses notification pop-ups while keeping notifications in the notification centre."
                    }
                }
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
                        required property string kind
                        required property string imageUrl
                        required property int index
                        width: ListView.view.width; height: 30; radius: 10; color: Theme.surface
                        Image {
                            visible: clipboardEntry.kind === "image" && clipboardEntry.imageUrl.length > 0
                            anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: 7
                            width: 22; height: 22
                            source: clipboardEntry.imageUrl
                            fillMode: Image.PreserveAspectCrop
                        }
                        Text {
                            anchors.fill: parent
                            anchors.leftMargin: clipboardEntry.kind === "image" ? 36 : 8
                            anchors.rightMargin: 8
                            verticalAlignment: Text.AlignVCenter
                            text: (clipboardEntry.kind === "files" ? "▤  " : "") + clipboardEntry.text
                            color: Theme.text; font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                        MouseArea { anchors.fill: parent; onClicked: Clipboard.restore(clipboardEntry.index) }
                    }
                }
                Text { visible: Clipboard.count === 0; text: "No copied text yet"; color: Theme.textDim; font.pixelSize: 10 }
            }

            Text {
                width: parent.width; wrapMode: Text.WordWrap
                text: "Wi-Fi and Bluetooth remain demonstration controls. Audio now tracks application streams and device changes; clipboard history retains text, images and file URLs. Android clipboard synchronisation requires the Android-side companion bridge."
                color: Theme.textDim; font.pixelSize: 10
            }
        }
    }
}
