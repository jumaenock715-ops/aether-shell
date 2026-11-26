import QtQuick

Rectangle {
    id: root
    property bool checked: false
    signal toggled(bool value)

    width: 44; height: 24; radius: 12
    color: checked ? Theme.accent : Theme.border
    Behavior on color { ColorAnimation { duration: Theme.animMs } }

    Rectangle {
        width: 18; height: 18; radius: 9
        y: 3
        x: root.checked ? parent.width - width - 3 : 3
        color: "white"
        Behavior on x { NumberAnimation { duration: Theme.animMs; easing.type: Easing.OutCubic } }
    }
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.toggled(!root.checked)
    }
}
