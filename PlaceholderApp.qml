import QtQuick

Item {
    property string appName: "App"
    property color tint: Theme.accent

    Column {
        anchors.centerIn: parent
        spacing: 12
        AppIcon { size: 72; tint: parent.parent.tint; label: parent.parent.appName; anchors.horizontalCenter: parent.horizontalCenter }
        Text { text: parent.parent.appName; color: Theme.text; font.pixelSize: 18; font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
        Text { text: "Placeholder window. The native app framework will host real apps here."; color: Theme.textDim; font.pixelSize: 12; anchors.horizontalCenter: parent.horizontalCenter }
    }
}
