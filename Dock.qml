import QtQuick

Rectangle {
    id: root
    property var openIds: []
    signal launch(string appId)
    signal launcherRequested()

    readonly property var pinned: Apps.list.filter(function (a) { return a.appId !== "notes" })

    width: Math.min(row.implicitWidth + 32, parent ? parent.width - 32 : row.implicitWidth + 32)
    height: 72
    radius: 24
    color: Theme.taskbar
    border.color: Theme.border
    border.width: 1

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 7

        // Aether launcher sits at the leading edge of the centred taskbar.
        Item {
            width: 48; height: 54
            Rectangle {
                anchors.centerIn: parent
                width: 42; height: 42; radius: 14
                color: startArea.containsMouse ? Theme.accent : Theme.surface
                border.color: startArea.containsMouse ? Theme.accent : Theme.border
                Behavior on color { ColorAnimation { duration: 130 } }
                Text {
                    anchors.centerIn: parent
                    text: "A"
                    color: startArea.containsMouse ? "white" : Theme.accent
                    font.pixelSize: 23; font.bold: true
                }
            }
            MouseArea {
                id: startArea; anchors.fill: parent
                hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                onClicked: root.launcherRequested()
            }
        }

        Repeater {
            model: root.pinned
            delegate: Item {
                width: 48; height: 54
                required property var modelData

                AppIcon {
                    size: 40
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: hover.containsMouse ? 1 : 5
                    tint: modelData.tint
                    label: modelData.name
                    iconName: modelData.icon !== undefined ? modelData.icon : ""
                    scale: hover.pressed ? 0.91 : (hover.containsMouse ? 1.10 : 1.0)
                    Behavior on scale { NumberAnimation { duration: 130; easing.type: Easing.OutCubic } }
                    Behavior on y { NumberAnimation { duration: 130; easing.type: Easing.OutCubic } }
                }

                Rectangle {
                    visible: root.openIds.indexOf(modelData.appId) >= 0
                    width: visible ? 14 : 4; height: 4; radius: 2
                    color: Theme.accent
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    Behavior on width { NumberAnimation { duration: 120 } }
                }

                MouseArea {
                    id: hover; anchors.fill: parent; hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.launch(modelData.appId)
                }
            }
        }
    }
}
