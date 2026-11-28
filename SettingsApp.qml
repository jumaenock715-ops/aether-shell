import QtQuick

Item {
    id: root

    Component.onCompleted: console.log("AETHER_SETTINGS_WINDOW_LOADED")

    Connections {
        target: AndroidRuntime
        function onOperationFailed(operation, reason) {
            Notifications.push("Android runtime", operation + ": " + reason)
        }
    }
    property int page: 0
    readonly property var pages: ["Appearance", "Display", "Network", "Notifications", "Audio", "Android", "About", "Phone", "Wi-Fi & hotspot", "More connections", "Users & accounts", "Security", "Storage", "Memory", "System information", "App management", "Battery lab"]
    readonly property var pageDescriptions: ["Personalise the Aether experience", "Brightness and visual behaviour", "Network overview", "Alerts, quiet mode and delivery", "Output, microphone and devices", "Android app compatibility layer", "Version and project information", "SIM, mobile network and calling", "Wi-Fi networks, hotspot and tethering", "USB, OTG, Bluetooth and wireless display", "Device users and online accounts", "Screen lock, permissions and credentials", "Internal storage and external media", "RAM usage and memory pressure", "Hardware, software and kernel details", "Installed apps, permissions and defaults", "Battery health, usage and charging"]
    readonly property var pageGroups: ["PERSONALISATION", "PERSONALISATION", "CONNECTIONS", "PERSONALISATION", "DEVICE", "DEVICE", "SYSTEM", "CONNECTIONS", "CONNECTIONS", "CONNECTIONS", "ACCOUNTS", "SECURITY", "DEVICE", "DEVICE", "SYSTEM", "APPS", "DEVICE"]
    readonly property var accents: ["#6D4AFF", "#14B8D4", "#3DDC97", "#FF5C7A", "#FFB020"]

    Rectangle {
        id: side
        width: 210; height: parent.height
        color: Theme.panel
        Column {
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 12 }
            spacing: 6
            Repeater {
                model: root.pages
                delegate: Column {
                    required property int index
                    required property string modelData
                    width: parent.width
                    spacing: 3
                    Text {
                        visible: index === 0 || root.pageGroups[index] !== root.pageGroups[index - 1]
                        text: root.pageGroups[index]
                        color: Theme.textDim
                        font.pixelSize: 9
                        font.bold: true
                        font.letterSpacing: 1.1
                        leftPadding: 10
                        topPadding: index === 0 ? 4 : 12
                    }
                    Rectangle {
                        width: parent.width
                        height: 52
                        radius: 14
                        color: root.page === index ? Theme.accent : "transparent"
                        Column {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: 12
                            anchors.rightMargin: 8
                            spacing: 2
                            Text {
                                text: modelData
                                color: root.page === index ? "white" : Theme.text
                                font.pixelSize: 13
                                font.bold: root.page === index
                            }
                            Text {
                                width: parent.width
                                text: root.pageDescriptions[index]
                                color: root.page === index ? "#E6E0FF" : Theme.textDim
                                font.pixelSize: 9
                                elide: Text.ElideRight
                            }
                        }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.page = index }
                    }
                }
            }
        }
    }

    Item {
        anchors { left: side.right; right: parent.right; top: parent.top; bottom: parent.bottom; leftMargin: 32; rightMargin: 28; topMargin: 26; bottomMargin: 24 }

        // Appearance
        Column {
            visible: root.page === 0; spacing: 18; width: parent.width
            Text { text: "Appearance"; color: Theme.text; font.pixelSize: 25; font.bold: true }
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
            Text { text: "Display"; color: Theme.text; font.pixelSize: 22; font.bold: true }
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

        // Notifications
        Column {
            visible: root.page === 3; spacing: 14; width: parent.width
            Text { text: "Notifications"; color: Theme.text; font.pixelSize: 18; font.bold: true }
            Row {
                spacing: 14
                Text { text: "Do not disturb"; color: Theme.text; font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter }
                Toggle { checked: SessionSettings.doNotDisturb; onToggled: (v) => SessionSettings.setDoNotDisturb(v) }
            }
            Text { text: "Desktop notification service"; color: Theme.text; font.pixelSize: 13; font.bold: true }
            Text {
                width: parent.width; wrapMode: Text.WordWrap
                text: "D-Bus status: " + Notifications.dbusStatus
                color: Notifications.dbusStatus === "active" ? "#3DDC97" : "#FFB020"
                font.pixelSize: 12
            }
            Text {
                width: parent.width; wrapMode: Text.WordWrap
                text: Notifications.dbusStatus === "active"
                    ? "Aether currently owns org.freedesktop.Notifications on the session bus."
                    : "The D-Bus notification name could not be registered. Another notification daemon may already own it, or the session bus may be unavailable."
                color: Theme.textDim; font.pixelSize: 11
            }
            Text {
                width: parent.width; wrapMode: Text.WordWrap
                text: "Notifications remain in the notification centre while Do Not Disturb suppresses toast popups."
                color: Theme.textDim; font.pixelSize: 11
            }
        }

        // Audio
        Column {
            visible: root.page === 4; spacing: 12; width: parent.width
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
            Text { text: "Application audio"; color: Theme.text; font.pixelSize: 13; font.bold: true }
            Repeater {
                model: Audio.streams
                delegate: Column {
                    required property var modelData
                    width: parent.width
                    spacing: 4
                    Text {
                        width: parent.width
                        text: modelData.name + " — " + modelData.provider
                        color: Theme.text; font.pixelSize: 11
                        elide: Text.ElideRight
                    }
                    Row {
                        spacing: 8
                        AetherSlider {
                            width: 190
                            value: modelData.volume / 100
                            onMoved: (v) => Audio.setStreamVolume(modelData.id, Math.round(v * 100))
                        }
                        Text {
                            text: modelData.volume + "%"
                            color: Theme.textDim; font.pixelSize: 10
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Toggle {
                            checked: modelData.muted
                            onToggled: (v) => Audio.setStreamMuted(modelData.id, v)
                        }
                    }
                }
            }
            Text {
                visible: Audio.streams.length === 0
                text: "No active application audio streams."
                color: Theme.textDim; font.pixelSize: 10
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
            visible: root.page === 5; spacing: 14; width: parent.width
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
            visible: root.page === 6; spacing: 10; width: parent.width
            Text { text: "About"; color: Theme.text; font.pixelSize: 18; font.bold: true }
            Text { text: "Aether Shell 0.1 (prototype)"; color: Theme.text; font.pixelSize: 14 }
            Text { text: "Qt " + Qt.platform.os + " build"; color: Theme.textDim; font.pixelSize: 12 }
            Text { width: parent.width; wrapMode: Text.WordWrap; color: Theme.textDim; font.pixelSize: 12
                   text: "First milestone of Project Aether OS: a desktop shell designed to become the real system shell." }
        }
        // Expanded device settings pages. Hardware-dependent values are not fabricated.
        Flickable {
            visible: root.page >= 7
            anchors.fill: parent
            contentWidth: width
            contentHeight: extraPages.height
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            Column {
                id: extraPages
                width: parent.width
                spacing: 14
                Text { text: root.pages[root.page]; color: Theme.text; font.pixelSize: 25; font.bold: true }
                Text {
                    width: parent.width
                    text: root.pageDescriptions[root.page]
                    color: Theme.textDim; font.pixelSize: 12; wrapMode: Text.WordWrap
                }
                Rectangle {
                    visible: root.page === 7
                    width: parent.width; height: 96; radius: 14
                    color: Theme.panel; border.color: Theme.border
                    Column {
                        anchors.fill: parent; anchors.margins: 14; spacing: 6
                        Text { text: "SIM & mobile network"; color: Theme.text; font.pixelSize: 14; font.bold: true }
                        Text {
                            width: parent.width; wrapMode: Text.WordWrap
                            text: "SIM status, mobile data, preferred network, roaming, access point names and calling services will be managed here when a modem service is integrated."
                            color: Theme.textDim; font.pixelSize: 11
                        }
                    }
                }
                Rectangle {
                    visible: root.page === 8
                    width: parent.width; height: 112; radius: 14
                    color: Theme.panel; border.color: Theme.border
                    Column {
                        anchors.fill: parent; anchors.margins: 14; spacing: 8
                        Row {
                            width: parent.width
                            Text { text: "Wi-Fi"; color: Theme.text; font.pixelSize: 14; font.bold: true }
                            Item { width: 1; height: 1 }
                            Toggle { checked: System.wifi; onToggled: (v) => System.wifi = v }
                        }
                        Text {
                            width: parent.width; wrapMode: Text.WordWrap
                            text: "Prototype switch only. Network scanning, saved networks, authentication, hotspot and tethering need NetworkManager and modem integration."
                            color: Theme.textDim; font.pixelSize: 11
                        }
                    }
                }
                Rectangle {
                    visible: root.page === 9
                    width: parent.width; height: 190; radius: 14
                    color: Theme.panel; border.color: Theme.border
                    Column {
                        anchors.fill: parent; anchors.margins: 14; spacing: 9
                        Text { text: "Connection options"; color: Theme.text; font.pixelSize: 14; font.bold: true }
                        Repeater {
                            model: ["USB connection mode", "USB OTG / host mode", "Bluetooth devices", "Wireless display / casting", "Ethernet and tethering"]
                            delegate: Text {
                                required property string modelData
                                text: modelData + "   ·   Service integration pending"
                                color: Theme.textDim; font.pixelSize: 12
                            }
                        }
                    }
                }
                Repeater {
                    model: root.page === 10 ? [
                        ["Device users", "User profiles, guest sessions and multi-user switching"],
                        ["Online accounts", "Account providers, sync status and account permissions"]
                    ] : root.page === 11 ? [
                        ["Screen lock", "PIN, password, pattern and automatic lock"],
                        ["Biometrics", "Fingerprint and face authentication, when supported"],
                        ["Permissions", "App access to location, camera, microphone and files"],
                        ["Credentials", "Certificates, keys and trusted credentials"]
                    ] : root.page === 12 ? [
                        ["Internal storage / ROM", "Internal flash capacity, used space and available space"],
                        ["External storage", "SD cards, USB drives and mounted volumes"],
                        ["Storage management", "Temporary files, downloads and cleanup"]
                    ] : root.page === 13 ? [
                        ["RAM", "Live memory usage, available memory and memory pressure"],
                        ["Memory management", "Per-process usage and background app limits"],
                        ["Swap and zRAM", "Swap space and compressed-memory status"]
                    ] : root.page === 14 ? [
                        ["Hardware", "CPU, architecture, graphics, memory and device identifiers"],
                        ["Software", "Aether OS version, Debian base, Qt version and build"],
                        ["Kernel", "Linux kernel release, boot parameters and uptime"]
                    ] : root.page === 15 ? [
                        ["Installed applications", "Browse, launch, uninstall and inspect installed apps"],
                        ["App permissions", "Review and manage application access"],
                        ["Default applications", "Choose browser, file manager and other defaults"],
                        ["Android applications", "Manage Android packages through the Android runtime"]
                    ] : root.page === 16 ? [
                        ["Battery status", "Charge level, charging state and power source"],
                        ["Battery health", "Capacity, cycle count and health estimates when exposed by hardware"],
                        ["Power usage", "Application and system energy consumption"],
                        ["Power modes", "Balanced, performance and battery saver policies"]
                    ] : []
                    delegate: Rectangle {
                        required property var modelData
                        width: extraPages.width; height: 70; radius: 12
                        color: Theme.panel; border.color: Theme.border
                        Column {
                            anchors.fill: parent; anchors.margins: 12; spacing: 5
                            Text { text: modelData[0]; color: Theme.text; font.pixelSize: 13; font.bold: true }
                            Text {
                                width: parent.width; wrapMode: Text.WordWrap
                                text: modelData[1]; color: Theme.textDim; font.pixelSize: 11
                            }
                        }
                    }
                }
                Text {
                    visible: root.page >= 10
                    width: parent.width; wrapMode: Text.WordWrap
                    text: "This page defines the Settings structure. Live values and actions will be connected as Aether's device services are implemented; unavailable hardware information is not fabricated."
                    color: Theme.textDim; font.pixelSize: 11
                }
            }
        }
    }
}
