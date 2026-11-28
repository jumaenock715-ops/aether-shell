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

    width: 660; height: 440
    radius: maximized ? 0 : Theme.radius
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
        width: parent.width; height: 38
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
            anchors.left: parent.left; anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            Rectangle {
                width: 14; height: 14; radius: 7; color: "#FF5C7A"
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.closeRequested() }
            }
            Rectangle {
                width: 14; height: 14; radius: 7; color: "#3DDC97"
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.toggleMaximize() }
            }
        }
    }

    // Content
    Loader {
        anchors { left: parent.left; right: parent.right; top: bar.bottom; bottom: parent.bottom }
        sourceComponent: root.appId === "settings" ? settingsComp : (root.appId === "files" ? filesComp : placeholderComp)
    }
    Component { id: settingsComp;    SettingsApp {} }
    Component { id: filesComp; FilesApp {} }
    Component {
        id: placeholderComp
        PlaceholderApp {
            appName: root.title
            tint: Apps.byId(root.appId) ? Apps.byId(root.appId).tint : Theme.accent
        }
    }

    // Click anywhere in the window to focus it
    MouseArea {
        anchors.fill: parent
        z: -1
        onPressed: (m) => { root.focusRequested(); m.accepted = false }
    }
}
