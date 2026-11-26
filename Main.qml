import QtQuick
import QtQuick.Window

Window {
    id: win
    width: 1280; height: 800
    minimumWidth: 800; minimumHeight: 560
    visible: true
    title: "Aether Shell"
    color: Theme.bg

    property bool launcherOpen: false
    property bool quickOpen: false
    property bool notifOpen: false
    property int topZ: 1

    ListModel { id: windows }

    readonly property var openIds: {
        var n = windows.count, r = []
        for (var i = 0; i < n; i++) r.push(windows.get(i).appId)
        return r
    }

    function closePopups() { launcherOpen = false; quickOpen = false; notifOpen = false }

    function raise(i) {
        topZ += 1
        windows.setProperty(i, "stack", topZ)
    }

    function openApp(appId) {
        var app = Apps.byId(appId)
        if (!app) return
        closePopups()
        for (var i = 0; i < windows.count; i++) {
            if (windows.get(i).appId === appId) { raise(i); return }
        }
        topZ += 1
        var n = windows.count
        windows.append({ appId: appId, title: app.name, stack: topZ,
                         startX: 120 + (n % 6) * 36, startY: 80 + (n % 6) * 32 })
    }

    // Wallpaper
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.dark ? "#141B3A" : "#DDE2F7" }
            GradientStop { position: 1.0; color: Theme.bg }
        }
        Rectangle { width: 620; height: 620; radius: 310; x: parent.width * 0.62; y: -160
                    color: Theme.accent; opacity: Theme.dark ? 0.16 : 0.18 }
        Rectangle { width: 480; height: 480; radius: 240; x: -120; y: parent.height * 0.55
                    color: Theme.accent2; opacity: Theme.dark ? 0.14 : 0.16 }
        Text {
            anchors.centerIn: parent
            text: "AETHER"
            color: Theme.text; opacity: 0.06
            font.pixelSize: 150; font.bold: true; font.letterSpacing: 18
        }
    }

    // Application windows
    Item {
        id: windowLayer
        anchors.fill: parent
        Repeater {
            model: windows
            delegate: AppWindow {
                required property int index
                required property string appId
                required property string title
                required property int stack
                required property int startX
                required property int startY

                x: startX; y: startY
                z: stack
                focused: stack === win.topZ
                onFocusRequested: win.raise(index)
                onCloseRequested: windows.remove(index)
            }
        }
    }

    TopBar {
        id: topBar
        anchors { left: parent.left; right: parent.right; top: parent.top }
        z: 1000
        onLauncherRequested: { var o = !win.launcherOpen; win.closePopups(); win.launcherOpen = o }
        onQuickSettingsRequested: { var o = !win.quickOpen; win.closePopups(); win.quickOpen = o }
        onNotificationsRequested: { var o = !win.notifOpen; win.closePopups(); win.notifOpen = o }
    }

    Dock {
        id: dock
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 14 }
        z: 1000
        openIds: win.openIds
        onLaunch: (appId) => win.openApp(appId)
        onLauncherRequested: { var o = !win.launcherOpen; win.closePopups(); win.launcherOpen = o }
    }

    // Popups
    Launcher {
        anchors { fill: parent; topMargin: Theme.barHeight }
        z: 1500
        visible: win.launcherOpen
        onLaunch: (appId) => win.openApp(appId)
        onCloseRequested: win.launcherOpen = false
    }
    QuickSettings {
        anchors.fill: parent; z: 1500
        visible: win.quickOpen
        onCloseRequested: win.quickOpen = false
    }
    NotificationCenter {
        anchors.fill: parent; z: 1500
        visible: win.notifOpen
        onCloseRequested: win.notifOpen = false
    }

    // Toast
    Rectangle {
        id: toast
        property string heading: ""
        property string message: ""
        width: 300; height: toastCol.height + 24
        x: parent.width - width - 12
        y: Theme.barHeight + 8
        z: 1400
        radius: 14; color: Theme.panel; border.color: Theme.border
        opacity: 0; visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 200 } }
        Column {
            id: toastCol
            anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: 14 }
            spacing: 3
            Text { text: toast.heading; color: Theme.text; font.pixelSize: 13; font.bold: true }
            Text { width: parent.width; text: toast.message; wrapMode: Text.WordWrap; color: Theme.textDim; font.pixelSize: 12 }
        }
        Timer { id: toastTimer; interval: 3500; onTriggered: toast.opacity = 0 }
    }
    Connections {
        target: Notifications
        function onPushed(title, body) {
            if (System.dnd || win.notifOpen) return
            toast.heading = title; toast.message = body
            toast.opacity = 1; toastTimer.restart()
        }
    }

    // Brightness overlay (demo): dims the whole shell
    Rectangle {
        anchors.fill: parent; z: 5000
        color: "black"
        opacity: (1 - System.brightness) * 0.7
        enabled: false
    }

    // Automated demo for CI: runs only when AETHER_DEMO_DIR is set.
    property int demoStep: 0
    property bool demoActed: false
    readonly property var demoSteps: [
        { name: "01-start",         act: function () { } },
        { name: "02-launcher",      act: function () { win.closePopups(); win.launcherOpen = true } },
        { name: "03-settings",      act: function () { win.openApp("settings") } },
        { name: "04-windows",       act: function () { win.openApp("files"); win.openApp("browser") } },
        { name: "05-quick-settings",act: function () { win.closePopups(); win.quickOpen = true } },
        { name: "06-notifications", act: function () { win.closePopups(); Notifications.push("Build", "Demo notification"); win.notifOpen = true } },
        { name: "07-light-theme",   act: function () { win.closePopups(); Theme.dark = false } }
    ]
    Timer {
        interval: 1300; repeat: true
        running: demoDir !== ""
        onTriggered: {
            if (win.demoStep >= win.demoSteps.length) { Qt.quit(); return }
            var s = win.demoSteps[win.demoStep]
            if (!win.demoActed) {
                s.act(); win.demoActed = true
            } else {
                var path = demoDir + "/" + s.name + ".png"
                win.contentItem.grabToImage(function (r) { r.saveToFile(path) })
                win.demoActed = false; win.demoStep += 1
            }
        }
    }

    Component.onCompleted: {
        Notifications.push("Welcome to Aether", "Open the launcher from the top-left button or the dock.")
    }
}
