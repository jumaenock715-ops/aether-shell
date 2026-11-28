import QtQuick

Item {
    id: root
    signal closeRequested()
    property string selectedSsid: ""
    property bool hiddenNetwork: false
    property bool enterpriseNetwork: false

    MouseArea { anchors.fill: parent; onClicked: root.closeRequested() }

    Rectangle {
        id: panel
        width: 340
        height: col.height + 36
        x: root.width - width - 12
        y: Theme.barHeight + 8
        radius: 20
        color: Theme.panel
        border.color: Theme.border

        MouseArea { anchors.fill: parent }

        Column {
            id: col
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 18 }
            spacing: 12

            Text { text: "Quick settings"; color: Theme.text; font.pixelSize: 15; font.bold: true }

            Grid {
                columns: 2
                spacing: 10
                width: parent.width
                property real cw: (width - spacing) / 2
                QuickTile {
                    width: parent.cw
                    label: "Wi-Fi"
                    subtitle: System.network && System.network.available
                              ? (System.network.activeSsid.length ? System.network.activeSsid : (System.wifi ? "Enabled" : "Off"))
                              : "NetworkManager unavailable"
                    on: System.wifi
                    onClicked: if (System.network) System.network.setWifiEnabled(!System.wifi)
                }
                QuickTile {
                    width: parent.cw
                    label: "Bluetooth"
                    subtitle: System.hardware && System.hardware.bluetoothAvailable ? (System.bluetooth ? "On" : "Off") : "BlueZ unavailable"
                    on: System.bluetooth
                    onClicked: if (System.hardware) System.hardware.setBluetoothPowered(!System.bluetooth)
                }
                QuickTile { width: parent.cw; label: "Dark mode"; subtitle: Theme.dark ? "On" : "Off"; on: Theme.dark; onClicked: Theme.dark = !Theme.dark }
                QuickTile { width: parent.cw; label: "Do not disturb"; subtitle: System.dnd ? "On" : "Off"; on: System.dnd; onClicked: System.dnd = !System.dnd }
            }

            Column {
                width: parent.width
                spacing: 5
                Row {
                    spacing: 10
                    Text { text: "Bluetooth controllers"; color: Theme.textDim; font.pixelSize: 12 }
                    Text { text: System.hardware && System.hardware.bluetoothControllers.length ? System.hardware.bluetoothControllers.join(", ") : "None detected"; color: Theme.textDim; font.pixelSize: 10; wrapMode: Text.WordWrap; width: parent.width - 90 }
                    Text { text: System.hardware && System.hardware.bluetoothScanning ? "Scanning…" : "Discover"; color: Theme.accent; font.pixelSize: 12; MouseArea { anchors.fill: parent; onClicked: if (System.hardware && !System.hardware.bluetoothScanning) System.hardware.scanBluetooth() } }
                }
                Repeater {
                    model: System.hardware ? System.hardware.bluetoothDeviceDetails : []
                    delegate: Row {
                        width: parent.width
                        spacing: 5
                        Text {
                            text: modelData.name + " (" + modelData.address + ")"
                                  + (modelData.connected ? " · Connected" : modelData.paired ? " · Paired" : " · Discovered")
                            color: Theme.text; font.pixelSize: 9; width: 95; elide: Text.ElideRight
                        }
                        Text {
                            text: modelData.paired ? "Paired" : "Pair"; color: Theme.accent; font.pixelSize: 10
                            MouseArea { anchors.fill: parent; onClicked: if (!modelData.paired && System.hardware) System.hardware.pairBluetooth(modelData.address) }
                        }
                        Text {
                            text: modelData.connected ? "Disconnect" : "Connect"; color: Theme.accent; font.pixelSize: 10
                            MouseArea { anchors.fill: parent; onClicked: if (System.hardware) { if (modelData.connected) System.hardware.disconnectBluetooth(modelData.address); else System.hardware.connectBluetooth(modelData.address) } }
                        }
                        Text {
                            text: modelData.trusted ? "Untrust" : "Trust"; color: Theme.accent; font.pixelSize: 10
                            MouseArea { anchors.fill: parent; onClicked: if (System.hardware && modelData.paired) System.hardware.trustBluetooth(modelData.address, !modelData.trusted) }
                        }
                        Text {
                            text: "Remove"; color: Theme.textDim; font.pixelSize: 10
                            MouseArea { anchors.fill: parent; onClicked: if (System.hardware && !modelData.connected) System.hardware.removeBluetooth(modelData.address) }
                        }
                    }
                }
            }

            Row {
                width: parent.width
                spacing: 8
                Text { text: "Available networks"; color: Theme.textDim; font.pixelSize: 12 }
                Text {
                    text: System.network && System.network.scanning ? "Scanning…" : "Scan"
                    color: Theme.accent
                    font.pixelSize: 12
                    MouseArea { anchors.fill: parent; onClicked: if (System.network) System.network.requestScan() }
                }
            }

            Column {
                width: parent.width
                spacing: 4
                Repeater {
                    model: System.network ? System.network.accessPoints : []
                    delegate: Rectangle {
                        width: col.width
                        height: 32
                        radius: 6
                        color: root.selectedSsid === modelData.ssid ? Theme.surface : "transparent"
                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            Text { text: modelData.ssid; color: Theme.text; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                            Item { width: 1; height: 1 }
                            Text { text: modelData.strength + "%"; color: Theme.textDim; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
                        }
                        MouseArea { anchors.fill: parent; onClicked: root.selectedSsid = modelData.ssid }
                    }
                }
                Text {
                    visible: System.network && !System.network.accessPoints.length
                    text: "No networks found. Try scanning."
                    color: Theme.textDim
                    font.pixelSize: 11
                }
            }

            Connections {
                target: System.network
                function onOperationFinished(success, message) {
                    if (success) wifiPassword.text = ""
                }
            }

            Column {
                width: parent.width
                spacing: 6
                Text {
                    text: root.hiddenNetwork ? "Connect to a hidden network" : "Connect to " + (root.selectedSsid || "Wi-Fi")
                    color: Theme.textDim; font.pixelSize: 12
                }
                Text {
                    text: root.hiddenNetwork ? "Use visible networks" : "Connect to hidden network"
                    color: Theme.accent; font.pixelSize: 11
                    MouseArea { anchors.fill: parent; onClicked: { root.hiddenNetwork = !root.hiddenNetwork; root.selectedSsid = ""; } }
                }
                TextInput {
                    id: hiddenSsid
                    width: parent.width; height: 30
                    visible: root.hiddenNetwork
                    placeholderText: "Network name (SSID)"
                    color: Theme.text; leftPadding: 8
                    verticalAlignment: TextInput.AlignVCenter
                    Rectangle { anchors.fill: parent; z: -1; radius: 6; color: Theme.surface; border.color: Theme.border }
                }
                Text {
                    visible: root.hiddenNetwork || root.selectedSsid.length > 0
                    text: root.enterpriseNetwork ? "Enterprise authentication: PEAP / MSCHAPv2" : "Use enterprise authentication (802.1X)"
                    color: Theme.accent; font.pixelSize: 11
                    MouseArea { anchors.fill: parent; onClicked: root.enterpriseNetwork = !root.enterpriseNetwork }
                }
                TextInput {
                    id: enterpriseUsername
                    width: parent.width; height: 30
                    visible: root.enterpriseNetwork && (root.hiddenNetwork || root.selectedSsid.length > 0)
                    placeholderText: "Enterprise username / identity"
                    color: Theme.text; leftPadding: 8
                    verticalAlignment: TextInput.AlignVCenter
                    Rectangle { anchors.fill: parent; z: -1; radius: 6; color: Theme.surface; border.color: Theme.border }
                }
                TextInput {
                    id: wifiPassword
                    width: parent.width; height: 30
                    visible: (root.hiddenNetwork || root.selectedSsid.length > 0) &&
                             (root.enterpriseNetwork || root.hiddenNetwork || (function() {
                                 var aps = System.network ? System.network.accessPoints : [];
                                 for (var i = 0; i < aps.length; ++i)
                                     if (aps[i].ssid === root.selectedSsid) return aps[i].secured;
                                 return true;
                             })())
                    echoMode: TextInput.Password
                    placeholderText: root.enterpriseNetwork ? "Enterprise password" : "Wi-Fi password"
                    color: Theme.text; leftPadding: 8
                    verticalAlignment: TextInput.AlignVCenter
                    Rectangle { anchors.fill: parent; z: -1; radius: 6; color: Theme.surface; border.color: Theme.border }
                }
                Text {
                    visible: (root.hiddenNetwork && hiddenSsid.text.length > 0) || (!root.hiddenNetwork && root.selectedSsid.length > 0)
                    text: System.network && System.network.connecting
                          ? "Connecting · " + System.network.connectionState : "Connect"
                    color: Theme.accent
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            var targetSsid = root.hiddenNetwork ? hiddenSsid.text.trim() : root.selectedSsid;
                            if (System.network && !System.network.connecting && targetSsid.length > 0)
                                System.network.connectToNetwork(targetSsid, wifiPassword.text,
                                                                 root.hiddenNetwork, enterpriseUsername.text,
                                                                 root.enterpriseNetwork);
                        }
                    }
                }
                Text {
                    visible: System.network && System.network.activeSsid.length > 0
                    text: "Disconnect from " + (System.network ? System.network.activeSsid : "")
                    color: Theme.accent
                    MouseArea { anchors.fill: parent; onClicked: if (System.network) System.network.disconnectFromNetwork() }
                }
            }

            Column {
                width: parent.width
                spacing: 5
                Text { text: "Display brightness"; color: Theme.textDim; font.pixelSize: 12 }
                AetherSlider {
                    width: parent.width
                    value: System.brightness
                    onMoved: (v) => { if (System.hardware) System.hardware.setBrightness(Math.round(v * 100)) }
                }
                Text { text: "Audio volume"; color: Theme.textDim; font.pixelSize: 12 }
                AetherSlider {
                    width: parent.width
                    value: System.hardware && System.hardware.volume >= 0 ? Math.min(1, System.hardware.volume / 100) : 0
                    onMoved: (v) => { if (System.hardware) System.hardware.setVolume(Math.round(v * 100)) }
                }
                Text {
                    text: System.hardware && System.hardware.muted ? "Unmute audio" : "Mute audio"
                    color: Theme.accent
                    MouseArea { anchors.fill: parent; onClicked: if (System.hardware) System.hardware.setMuted(!System.hardware.muted) }
                }
            }

            Column {
                width: parent.width
                spacing: 5
                Text {
                    text: System.hardware && System.hardware.batteryPresent ? "Battery: " + System.battery + "% · " + System.batteryStatus : "No battery detected"
                    color: Theme.textDim
                    font.pixelSize: 11
                }
                Repeater {
                    model: System.hardware ? System.hardware.batteryDevices : []
                    delegate: Text { text: modelData; color: Theme.textDim; font.pixelSize: 10 }
                }
                Row {
                    spacing: 12
                    Text { text: "Suspend"; color: Theme.accent; MouseArea { anchors.fill: parent; onClicked: if (System.hardware) System.hardware.suspend() } }
                    Text { text: "Restart"; color: Theme.accent; MouseArea { anchors.fill: parent; onClicked: if (System.hardware) System.hardware.restart() } }
                    Text { text: "Shut down"; color: Theme.accent; MouseArea { anchors.fill: parent; onClicked: if (System.hardware) System.hardware.shutdown() } }
                }
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: System.network && System.network.lastError.length
                      ? System.network.lastError
                      : (System.hardware && System.hardware.lastError.length ? System.hardware.lastError : "Hardware access depends on installed Linux services and device permissions.")
                color: Theme.textDim
                font.pixelSize: 11
            }
        }
    }
}
