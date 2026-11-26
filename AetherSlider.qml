import QtQuick

Item {
    id: root
    property real value: 0.5
    signal moved(real value)

    implicitWidth: 200; implicitHeight: 24

    Rectangle {
        id: track
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width; height: 6; radius: 3
        color: Theme.border
        Rectangle {
            width: parent.width * root.value; height: parent.height; radius: 3
            color: Theme.accent
        }
    }
    Rectangle {
        width: 18; height: 18; radius: 9; color: "white"
        anchors.verticalCenter: parent.verticalCenter
        x: (root.width - width) * root.value
    }
    MouseArea {
        anchors.fill: parent
        function setFrom(mx) {
            root.moved(Math.max(0.05, Math.min(1, mx / root.width)))
        }
        onPressed: (m) => setFrom(m.x)
        onPositionChanged: (m) => { if (pressed) setFrom(m.x) }
    }
}
