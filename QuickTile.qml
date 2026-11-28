import QtQuick

Rectangle {
    id: root
    property string label
    property string subtitle
    property bool on: false
    signal clicked()

    height: 76; radius: 18
    color: on ? Theme.accent : Theme.surface
    border.color: root.on ? Qt.darker(Theme.accent, 1.12) : Theme.border
    Behavior on color { ColorAnimation { duration: Theme.animMs } }

    Column {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left; anchors.leftMargin: 16
        spacing: 4
        Text { text: root.label; color: root.on ? "white" : Theme.text; font.pixelSize: 14; font.bold: true }
        Text { text: root.subtitle; color: root.on ? "#E6E0FF" : Theme.textDim; font.pixelSize: 12 }
    }
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
