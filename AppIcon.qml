import QtQuick

Item {
    id: root
    property color tint: Theme.accent
    property string label: "?"
    property string iconName: ""
    property int size: 56

    width: size; height: size

    Image {
        id: img
        anchors.fill: parent
        source: root.iconName !== "" ? "image://aethericon/" + root.iconName : ""
        sourceSize: Qt.size(root.size * 2, root.size * 2)
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        smooth: true
    }

    Rectangle {
        anchors.fill: parent
        visible: img.status !== Image.Ready
        radius: root.size * 0.28
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.lighter(root.tint, 1.25) }
            GradientStop { position: 1.0; color: root.tint }
        }
        Text {
            anchors.centerIn: parent
            text: root.label.charAt(0).toUpperCase()
            color: "white"
            font.pixelSize: root.size * 0.46
            font.bold: true
        }
    }
}
