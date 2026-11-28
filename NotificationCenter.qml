import QtQuick

Item {
    id: root
    signal closeRequested()

    MouseArea { anchors.fill: parent; onClicked: root.closeRequested() }

    Rectangle {
        id: panel
        width: Math.min(410, root.width - 24)
        x: root.width - width - 12
        y: Theme.barHeight + 8
        height: root.height - y - 100
        radius: 26
        color: Theme.launcherGlass; border.color: Theme.border

        MouseArea { anchors.fill: parent }

        Item {
            id: header
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 20 }
            height: 34
            Text { text: "Notifications"; color: Theme.text; font.pixelSize: 18; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
            Text {
                text: "Clear all"; color: Theme.accent2; font.pixelSize: 12
                anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                visible: Notifications.count > 0
                MouseArea { anchors.fill: parent; anchors.margins: -6; cursorShape: Qt.PointingHandCursor; onClicked: Notifications.clear() }
            }
        }

        ListView {
            id: list
            anchors { left: parent.left; right: parent.right; top: header.bottom; bottom: footer.top; margins: 18 }
            anchors.topMargin: 14
            clip: true; spacing: 8
            model: Notifications.groupedItems
            section.property: "appName"
            section.criteria: ViewSection.FullString
            section.delegate: Rectangle {
                required property string section
                width: list.width
                height: 26
                color: "transparent"
                Row {
                    anchors.fill: parent
                    spacing: 8
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: section.length > 0 ? section : "Aether"
                        color: Theme.text
                        font.pixelSize: 12
                        font.bold: true
                    }
                    Rectangle { anchors.verticalCenter: parent.verticalCenter; width: 5; height: 5; radius: 3; color: Theme.accent2 }
                }
            }
            delegate: Rectangle {
                required property string appName
                required property string title
                required property string body
                required property string time
                required property int notificationId
                required property var actions
                width: list.width
                height: card.height + 26
                radius: 18; color: Theme.surface; border.color: Theme.border
                Column {
                    id: card
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: 16 }
                    spacing: 3
                    Row {
                        width: parent.width
                        Text { width: parent.width - 20; visible: appName.length > 0; text: appName; color: Theme.textDim; font.pixelSize: 10; elide: Text.ElideRight }
                        Text {
                            text: "×"; color: Theme.textDim; font.pixelSize: 16
                            MouseArea {
                                anchors.fill: parent; anchors.margins: -4
                                cursorShape: Qt.PointingHandCursor
                                onClicked: Notifications.closeNotification(notificationId, 2)
                            }
                        }
                    }
                    Row {
                        spacing: 8
                        Text { text: title; color: Theme.text; font.pixelSize: 14; font.bold: true }
                        Text { text: time; color: Theme.textDim; font.pixelSize: 11 }
                    }
                    Text { width: parent.width; text: body; wrapMode: Text.WordWrap; color: Theme.textDim; font.pixelSize: 12 }
                    Row {
                        spacing: 8
                        visible: actions && actions.length >= 2
                        Repeater {
                            model: actions ? Math.floor(actions.length / 2) : 0
                            delegate: Rectangle {
                                required property int index
                                width: actionLabel.implicitWidth + 18; height: 30; radius: 10
                                color: Theme.panel; border.color: Theme.border
                                Text { id: actionLabel; anchors.centerIn: parent; text: actions[index * 2 + 1]; color: Theme.accent2; font.pixelSize: 11 }
                                MouseArea {
                                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: Notifications.invokeAction(notificationId, actions[index * 2])
                                }
                            }
                        }
                    }
                }
            }
            Text {
                visible: Notifications.count === 0
                anchors.centerIn: parent
                text: "You're all caught up"
                color: Theme.textDim; font.pixelSize: 13
            }
        }

        Rectangle {
            id: footer
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 18 }
            height: 46; radius: 23; color: Theme.accent
            Text { anchors.centerIn: parent; text: "Send test notification"; color: "white"; font.pixelSize: 13; font.bold: true }
            MouseArea {
                anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                onClicked: Notifications.push("Aether Shell", "This is a test notification.")
            }
        }
    }
}
