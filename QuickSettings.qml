import QtQuick

Item {
    id: root
    signal closeRequested()\n    property string selectedSsid: ""

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
                QuickTile {
                    width: parent.cw
                    label: "Wi-Fi"
                    subtitle: System.network && System.network.available
                              ? (System.network.activeSsid.length
                                 ? System.network.activeSsid : (System.wifi ? "Enabled" : "Off"))
                              : "NetworkManager unavailable"
                    on: System.wifi
                    onClicked: if (System.network) System.network.setWifiEnabled(!System.wifi)
                }
                QuickTile {
                    width: parent.cw
                    label: "Bluetooth"
                    subtitle: "Not connected"
                    on: false
                    onClicked: { }
                }
                QuickTile { width: parent.cw; label: "Dark mode"; subtitle: Theme.dark ? "On" : "Off"; on: Theme.dark; onClicked: Theme.dark = !Theme.dark }
                QuickTile { width: parent.cw; label: "Do not disturb"; subtitle: System.dnd ? "On" : "Off"; on: System.dnd; onClicked: System.dnd = !System.dnd }
            }

            Row {
                width: parent.width
                spacing: 8
                Text {
                    text: "Available networks"
                    color: Theme.textDim
                    font.pixelSize: 12
                    anchors.verticalCenter: parent.verticalCenter
                }
                Item { width: 1; height: 1 }
                Text {
                    text: System.network && System.network.scanning ? "Scanning…" : "Scan"
                    color: Theme.text
                    font.pixelSize: 12
                    anchors.verticalCenter: parent.verticalCenter
                    MouseArea {
                        anchors.fill: parent
                        onClicked: if (System.network) System.network.requestScan()
                    }
                }
            }

            Column {
                width: parent.width
                spacing: 5
                Repeater {
                    model: System.network ? System.network.accessPoints : []
                    delegate: Rectangle {
                        width: col.width
                        height: 34
                        radius: 7
                        color: "transparent"
                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 8
                            Text {
                                text: modelData.ssid
                                color: Theme.text
                                font.pixelSize: 12
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Item { width: 1; height: 1 }
                            Text {
                                text: modelData.strength + "%"
                                color: Theme.textDim
                                font.pixelSize: 11
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                }
                Text {
                    visible: System.network && !System.network.accessPoints.length
                    text: "No networks found. Try scanning."
                    color: Theme.textDim
                    font.pixelSize: 11
                }
            }

            Column {
                width: parent.width
                spacing: 6
                Text { text: "Display brightness"; color: Theme.textDim; font.pixelSize: 12 }
                AetherSlider {
                    width: parent.width
                    value: System.brightness
                    onMoved: (v) => System.brightness = v
                }
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: System.network && System.network.lastError.length
                      ? System.network.lastError
                      : "Wi-Fi controls use NetworkManager. Bluetooth, brightness and other controls are not integrated yet."
                color: Theme.textDim
                font.pixelSize: 11
            }
        }
    }
}
