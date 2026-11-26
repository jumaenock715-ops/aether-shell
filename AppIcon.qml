import QtQuick

Rectangle {
    id: root
    property color tint: Theme.accent
    property string label: "?"
    property int size: 56

    width: size; height: size
    radius: size * 0.28
    gradient: Gradient {
        GradientStop { position: 0.0; color: Qt.lighter(root.tint, 1.25) }
        GradientStop { position: 1.0; color: root.tint }
    }
    Text {
        anchors.centerIn: parent
        text: root.label.charAt(0)
        color: "white"
        font.pixelSize: root.size * 0.46
        font.bold: true
    }
}
