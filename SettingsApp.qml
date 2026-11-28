import QtQuick

Item {
    id: root

    Connections {
        target: AndroidRuntime
        function onOperationFailed(operation, reason) {
            Notifications.push("Android runtime", operation + ": " + reason)
        }
    }
    property int page: 0
    readonly property var pages: ["Appearance", "Display", "Network", "Audio", "Android", "About"]
    readonly property var accents: ["#6D4AFF", "#14B8D4", "#3DDC97", "#FF5C7A", "#FFB020"]

    Rectangle {
        id: side
        width: 160; height: parent.height
        color: Theme.surface
        Column {
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 10 }
            spacing: 4
            Repeater {
                model: root.pages
                delegate: Rectangle {
                    required property int index
                    required property string modelData
                    width: parent.width; height: 34; radius: 8
                    color: root.page === index ? Theme.accent : "transparent"
                    Text { anchors.verticalCenter: parent.verticalCenter; x: 12; text: modelData
                           color: root.page === index ? "white" : Theme.text; font.pixelSize: 13 }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.page = index }
                }
            }
        }
    }

    Item {
        anchors { left: side.right; right: parent.right; top: parent.top; bottom: parent.bottom; margins: 24 }

        // Appearance
        Column {
            visible: root.page === 0; spacing: 18; width: parent.width
            Text { text: "Appearance"; color: Theme.text; font.pixelSize: 18; font.bold: true }
            Row {
                spacing: 16
                Text { text: "Dark mode"; color: Theme.text; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
                Toggle { checked: Theme.dark; onToggled: (v) => SessionSettings.setDarkMode(v) }
            }
            Column {
                spacing: 8
                Text { text: "Accent colour"; color: Theme.text; font.pixelSize: 14 }
                Row {
                    spacing: 10
                    Repeater {
                        model: root.accents
                        delegate: Rectangle {
                            required property string modelData
                            width: 32; height: 32; radius: 16; color: modelData
                            border.width: Theme.accent.toString().toLowerCase() === modelData.toLowerCase() ? 3 : 0
                            border.color: Theme.text
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                        onClicked: SessionSettings.setAccentColor(modelData) }
                        }
                    }
                }
            }
        }

        // Display
        Column {
            visible: root.page === 1; spacing: 18; width: parent.width
            Text { text: "Display"; color: Theme.text; font.pixelSize: 18; font.bold: true }
            Text { text: "Brightness"; color: Theme.text; font.pixelSize: 14 }
            AetherSlider { width: 260; value: System.brightness; onMoved: (v) => System.brightness = v }
        }

        // Network
        Column {
            visible: root.page === 2; spacing: 18; width: parent.width
            Text { text: "Network"; color: Theme.text; font.pixelSize: 18; font.bold: true }
            Row { spacing: 16
                Text { text: "Wi-Fi"; color: Theme.text; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
                Toggle { checked: System.wifi; onToggled: (v) => System.wifi = v } }
            Row { spacing: 16
                Text { text: "Bluetooth"; color: Theme.text; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
                Toggle { checked: System.bluetooth; onToggled: (v) => System.bluetooth = v } }
            Text { text: "Demo toggles. Real networking arrives with the Aether system services."; color: Theme.textDim; font.pixelSize: 12 }
        }

        // Audio
        Column {
            visible: root.page === 3; spacing: 12; width: parent.width
            Text { text: "Audio"; color: Theme.text; font.pixelSize: 18; font.bold: true }
            Text {
                text: Audio.available ? "Audio control service available" : "No supported audio control utility detected."
                color: Theme.textDim; font.pixelSize: 11
            }
            Row {
                spacing: 10
                Text { text: "Output volume"; color: Theme.text; font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter }
                AetherSlider { width: 170; value: Audio.volume / 100; onMoved: (v) => Audio.setVolume(Math.round(v * 100)) }
                Text { text: Audio.volume + "%"; color: Theme.textDim; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
            }
            Row {
                spacing: 12
                Text { text: "Mute output"; color: Theme.text; font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter }
                Toggle { checked: Audio.muted; onToggled: (v) => Audio.setMuted(v) }
            }
            Text { text: "Output devices"; color: Theme.text; font.pixelSize: 13; font.bold: true }
            Repeater {
                model: Audio.outputs
                delegate: Rectangle {
                    required property var modelData
                    width: parent.width; height: 30; radius: 6
                    color: modelData.name === Audio.selectedOutput ? Theme.surface : "transparent"
                    Text { anchors.verticalCenter: parent.verticalCenter; x: 8; text: modelData.name; color: Theme.text; font.pixelSize: 11 }
                    MouseArea { anchors.fill: parent; onClicked: Audio.selectOutput(modelData.name) }
                }
            }
            Text { text: "Microphone level"; color: Theme.text; font.pixelSize: 13; font.bold: true }
            Row {
                spacing: 10
                AetherSlider { width: 170; value: Audio.inputVolume / 100; onMoved: (v) => Audio.setInputVolume(Math.round(v * 100)) }
                Text { text: Audio.inputVolume + "%"; color: Theme.textDim; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
                Toggle { checked: Audio.inputMuted; onToggled: (v) => Audio.setInputMuted(v) }
            }
            Text { text: "Input devices"; color: Theme.text; font.pixelSize: 13; font.bold: true }
            Repeater {
                model: Audio.inputs
                delegate: Rectangle {
                    required property var modelData
                    width: parent.width; height: 30; radius: 6
                    color: modelData.name === Audio.selectedInput ? Theme.surface : "transparent"
                    Text { anchors.verticalCenter: parent.verticalCenter; x: 8; text: modelData.name; color: Theme.text; font.pixelSize: 11 }
                    MouseArea { anchors.fill: parent; onClicked: Audio.selectInput(modelData.name) }
                }
            }
            Text { text: "Refresh devices"; color: Theme.accent2; font.pixelSize: 12 }
            MouseArea { width: 100; height: 22; onClicked: Audio.refresh() }
            Text {
                width: parent.width; wrapMode: Text.WordWrap
                text: "Device discovery and selection use pactl. Volume and mute use wpctl with pactl fallback. Per-application routing, audio capture and Android audio bridging remain future integration work."
                color: Theme.textDim; font.pixelSize: 10
            }
        }

        // Android runtime
        Column {
            visible: root.page === 4; spacing: 14; width: parent.width
            Text { text: "Android runtime"; color: Theme.text; font.pixelSize: 18; font.bold: true }
            Text {
                width: parent.width; wrapMode: Text.WordWrap
                text: AndroidRuntime.available
                    ? "Waydroid command-line tools detected."
                    : "Waydroid is not installed or is not available in the system PATH."
                color: Theme.text; font.pixelSize: 13
            }
            Text {
                text: "Session state: " + AndroidRuntime.runtimeState
                color: Theme.textDim; font.pixelSize: 12
            }
            Row {
                spacing: 10
                Rectangle {
                    width: 112; height: 36; radius: 8
                    color: AndroidRuntime.available ? Theme.accent : Theme.surface
                    opacity: AndroidRuntime.available ? 1.0 : 0.45
                    Text { anchors.centerIn: parent; text: "Start session"; color: "white"; font.pixelSize: 12 }
                    MouseArea {
                        anchors.fill: parent
                        enabled: AndroidRuntime.available
                        onClicked: AndroidRuntime.requestStartRuntime()
                    }
                }
                Rectangle {
                    width: 112; height: 36; radius: 8
                    color: Theme.surface; border.color: Theme.border
                    opacity: AndroidRuntime.available ? 1.0 : 0.45
                    Text { anchors.centerIn: parent; text: "Stop session"; color: Theme.text; font.pixelSize: 12 }
                    MouseArea {
                        anchors.fill: parent
                        enabled: AndroidRuntime.available
                        onClicked: AndroidRuntime.requestStopRuntime()
                    }
                }
                Rectangle {
                    width: 92; height: 36; radius: 8
                    color: Theme.surface; border.color: Theme.border
                    Text { anchors.centerIn: parent; text: "Refresh apps"; color: Theme.text; font.pixelSize: 12 }
                    MouseArea {
                        anchors.fill: parent
                        enabled: AndroidRuntime.available
                        onClicked: AndroidRuntime.refreshApplications()
                    }
                }
            }
            Text {
                width: parent.width; wrapMode: Text.WordWrap
                text: "Android applications appear in the launcher when a Waydroid session is available. This control does not install Waydroid or initialize its Android image."
                color: Theme.textDim; font.pixelSize: 11
            }
            Text {
                text: "Android launch requests"
                color: Theme.text; font.pixelSize: 13; font.bold: true
            }
            Column {
                width: parent.width; spacing: 7
                Repeater {
                    model: AndroidSessions
                    delegate: Column {
                        required property string name
                        required property string state
                        required property string activityState
                        required property string windowState
                        required property int windowCount
                        required property int activeWindowCount
                        width: parent.width; spacing: 2
                        Text {
                            width: parent.width; wrapMode: Text.WordWrap
                            text: name + " — " + state
                            color: Theme.text; font.pixelSize: 12
                        }
                        Text {
                            width: parent.width; wrapMode: Text.WordWrap
                            text: "Activity state: " + activityState
                            color: Theme.textDim; font.pixelSize: 10
                        }
                        Text {
                            width: parent.width; wrapMode: Text.WordWrap
                            text: "Windows: " + windowCount + " (active: " + activeWindowCount + ") — " + windowState
                            color: Theme.textDim; font.pixelSize: 10
                        }
                    }
                }
                Text {
                    visible: AndroidSessions.count === 0
                    text: "No Android launch requests recorded in this session."
                    color: Theme.textDim; font.pixelSize: 11
                }
            }
        }

        // About
        Column {
            visible: root.page === 5; spacing: 10; width: parent.width
            Text { text: "About"; color: Theme.text; font.pixelSize: 18; font.bold: true }
            Text { text: "Aether Shell 0.1 (prototype)"; color: Theme.text; font.pixelSize: 14 }
            Text { text: "Qt " + Qt.platform.os + " build"; color: Theme.textDim; font.pixelSize: 12 }
            Text { width: parent.width; wrapMode: Text.WordWrap; color: Theme.textDim; font.pixelSize: 12
                   text: "First milestone of Project Aether OS: a desktop shell designed to become the real system shell." }
        }
    }
}
