import QtQuick

Rectangle {
    id: root
    property string label
    property string subtitle
    property bool on: false
    property bool expanded: false
    signal clicked()
    signal detailsClicked()

    height: expanded ? 104 : 76
    radius: 18
    color: on ? Theme.accent : Theme.surface
    border.color: on ? Qt.darker(Theme.accent, 1.12) : Theme.border
    Behavior on color { ColorAnimation { duration: Theme.animMs } }
    Behavior on height { NumberAnimation { duration: Theme.animMs; easing.type: Easing.OutCubic } }

    MouseArea {
        z: -1
        anchors.fill: parent
        anchors.rightMargin: root.expanded ? 54 : 0
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 14
        spacing: 4
        Text { text: root.label; color: root.on ? "white" : Theme.text; font.pixelSize: 14; font.bold: true }
        Text { text: root.subtitle; color: root.on ? "#E6E0FF" : Theme.textDim; font.pixelSize: 12 }
        Item {
            width: parent.width
            height: root.expanded ? 22 : 0
            visible: root.expanded
            Text { anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; text: "Tap tile to toggle"; color: root.on ? "#E6E0FF" : Theme.textDim; font.pixelSize: 10 }
            Text {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: "Less"
                color: root.on ? "white" : Theme.accent2
                font.pixelSize: 11
                font.bold: true
                MouseArea { anchors.fill: parent; anchors.margins: -6; cursorShape: Qt.PointingHandCursor; onClicked: root.detailsClicked() }
            }
        }
    }
    Text {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 14
        anchors.bottomMargin: 10
        visible: !root.expanded
        text: "Details  ›"
        color: root.on ? "#E6E0FF" : Theme.accent2
        font.pixelSize: 10
        MouseArea { anchors.fill: parent; anchors.margins: -7; cursorShape: Qt.PointingHandCursor; onClicked: root.detailsClicked() }
    }
}