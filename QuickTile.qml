import QtQuick

Rectangle {
    id: root
    property string label
    property string subtitle
    property bool on: false
    signal clicked()

    height: 62; radius: 12
    color: on ? Theme.accent : Theme.surface
    border.color: Theme.border
    Behavior on color { ColorAnimation { duration: Theme.animMs } }

    Column {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left; anchors.leftMargin: 14
        spacing: 2
        Text { text: root.label; color: root.on ? "white" : Theme.text; font.pixelSize: 13; font.bold: true }
        Text { text: root.subtitle; color: root.on ? "#E6E0FF" : Theme.textDim; font.pixelSize: 11 }
    }
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
