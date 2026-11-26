import QtQuick

Rectangle {
    id: root
    property var openIds: []
    signal launch(string appId)
    signal launcherRequested()

    readonly property var pinned: Apps.list.filter(function (a) { return a.appId !== "notes" })

    width: row.width + 24
    height: 68
    radius: 22
    color: Theme.panel
    border.color: Theme.border

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 12

        Repeater {
            model: root.pinned
            delegate: Item {
                width: 48; height: 56
                required property var modelData

                AppIcon {
                    id: icon
                    size: 46
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: hover.containsMouse ? 0 : 4
                    tint: modelData.tint
                    label: modelData.name
                    scale: hover.pressed ? 0.92 : (hover.containsMouse ? 1.12 : 1.0)
                    Behavior on scale { NumberAnimation { duration: 120 } }
                    Behavior on y { NumberAnimation { duration: 120 } }
                }
                Rectangle {   // running indicator
                    visible: root.openIds.indexOf(modelData.appId) >= 0
                    width: 5; height: 5; radius: 2.5; color: Theme.accent2
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                }
                MouseArea {
                    id: hover; anchors.fill: parent; hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.launch(modelData.appId)
                }
            }
        }

        Item {
            width: 1; height: 56
            Rectangle { width: 1; height: 40; y: 8; color: Theme.border }
        }

        Item {
            width: 48; height: 56
            Rectangle {
                width: 46; height: 46; radius: 13
                y: 4
                color: Theme.surface; border.color: Theme.border
                Text { anchors.centerIn: parent; text: "All"; color: Theme.text; font.pixelSize: 13; font.bold: true }
            }
            MouseArea {
                anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                onClicked: root.launcherRequested()
            }
        }
    }
}
