import QtQuick

Rectangle {
    id: root
    property string appId
    property string title
    property bool focused: false
    property bool maximized: false
    property rect restoreGeometry: Qt.rect(0, 0, 0, 0)

    signal closeRequested()
    signal focusRequested()
    signal minimizeRequested()

    width: 660; height: 440
    radius: maximized ? 0 : 18
    color: Theme.surfaceStrong
    border.color: focused ? Theme.accent : Theme.border
    border.width: focused ? 2 : 1
    clip: true

    // Open animation
    opacity: 0; scale: 0.94
    Component.onCompleted: openAnim.start()
    ParallelAnimation {
        id: openAnim
        NumberAnimation { target: root; property: "opacity"; to: 1; duration: Theme.animMs }
        NumberAnimation { target: root; property: "scale"; to: 1; duration: Theme.animMs; easing.type: Easing.OutCubic }
    }

    function toggleMaximize() {
        if (!maximized) {
            restoreGeometry = Qt.rect(x, y, width, height)
            x = 0; y = Theme.barHeight
            width = parent.width; height = parent.height - Theme.barHeight
            maximized = true
        } else {
            x = restoreGeometry.x; y = restoreGeometry.y
            width = restoreGeometry.width; height = restoreGeometry.height
            maximized = false
        }
    }

    // Title bar
    Rectangle {
        id: bar
        width: parent.width; height: 44
        color: Theme.surface

        MouseArea {
            anchors.fill: parent
            drag.target: root.maximized ? null : root
            drag.minimumX: -root.width + 120
            drag.maximumX: root.parent ? root.parent.width - 120 : 0
            drag.minimumY: Theme.barHeight
            drag.maximumY: root.parent ? root.parent.height - 60 : 0
            onPressed: root.focusRequested()
            onDoubleClicked: root.toggleMaximize()
        }

        Text {
            anchors.centerIn: parent
            text: root.title
            color: Theme.text; font.pixelSize: 13; font.bold: true
        }

        Row {
            anchors.right: parent.right; anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            spacing: 5
            Rectangle {
                width: 34; height: 28; radius: 7
                color: minimizeArea.containsMouse ? Theme.border : "transparent"
                Text { anchors.centerIn: parent; text: "—"; color: Theme.text; font.pixelSize: 13 }
                MouseArea { id: minimizeArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: root.minimizeRequested() }
            }
            Rectangle {
                width: 34; height: 28; radius: 7
                color: maximizeArea.containsMouse ? Theme.border : "transparent"
                Text { anchors.centerIn: parent; text: root.maximized ? "❐" : "□"; color: Theme.text; font.pixelSize: 13 }
                MouseArea { id: maximizeArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: root.toggleMaximize() }
            }
            Rectangle {
                width: 42; height: 28; radius: 7
                color: closeArea.containsMouse ? "#C94355" : "transparent"
                Text { anchors.centerIn: parent; text: "×"; color: closeArea.containsMouse ? "white" : Theme.text; font.pixelSize: 19 }
                MouseArea { id: closeArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: root.closeRequested() }
            }
        }
    }

    // Application content is selected by the stable built-in app ID.
    // Avoid Loader/sourceComponent routing here: Settings must never fall
    // through to the generic placeholder because of a title or catalogue alias.
    SettingsApp {
        id: settingsContent
        anchors { left: parent.left; right: parent.right; top: bar.bottom; bottom: parent.bottom }
        visible: root.appId === "settings"
    }

    FilesApp {
        id: filesContent
        anchors { left: parent.left; right: parent.right; top: bar.bottom; bottom: parent.bottom }
        visible: root.appId === "files"
    }

    PlaceholderApp {
        anchors { left: parent.left; right: parent.right; top: bar.bottom; bottom: parent.bottom }
        visible: root.appId !== "settings" && root.appId !== "files"
        appName: root.title
        tint: Apps.byId(root.appId) ? Apps.byId(root.appId).tint : Theme.accent
    }

    // Click anywhere in the window to focus it
    MouseArea {
        anchors.fill: parent
        z: -1
        onPressed: (m) => { root.focusRequested(); m.accepted = false }
    }
}
