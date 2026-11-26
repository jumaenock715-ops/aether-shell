import QtQuick

Item {
    id: root
    signal launch(string appId)
    signal closeRequested()

    property string query: ""
    readonly property var results: Apps.list.filter(function (a) {
        return a.name.toLowerCase().indexOf(query.toLowerCase()) >= 0
    })

    onVisibleChanged: if (visible) { query = ""; search.text = ""; search.forceActiveFocus() }

    // Scrim
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.55)
        MouseArea { anchors.fill: parent; onClicked: root.closeRequested() }
    }

    Rectangle {
        id: panel
        anchors.centerIn: parent
        width: Math.min(720, root.width - 60)
        height: Math.min(520, root.height - 120)
        radius: 24
        color: Theme.panel
        border.color: Theme.border

        // Search field
        Rectangle {
            id: searchBox
            anchors { top: parent.top; left: parent.left; right: parent.right; margins: 24 }
            height: 44; radius: 22
            color: Theme.surface; border.color: search.activeFocus ? Theme.accent : Theme.border

            TextInput {
                id: search
                anchors.fill: parent; anchors.leftMargin: 20; anchors.rightMargin: 20
                verticalAlignment: TextInput.AlignVCenter
                color: Theme.text; font.pixelSize: 15
                selectionColor: Theme.accent
                clip: true
                onTextChanged: root.query = text
                Keys.onEscapePressed: root.closeRequested()
                Keys.onReturnPressed: if (root.results.length > 0) root.launch(root.results[0].appId)
                Text {
                    visible: search.text.length === 0
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Search apps..."
                    color: Theme.textDim; font.pixelSize: 15
                }
            }
        }

        Flow {
            anchors { top: searchBox.bottom; left: parent.left; right: parent.right; bottom: parent.bottom; margins: 24 }
            anchors.topMargin: 28
            spacing: 22
            Repeater {
                model: root.results
                delegate: Item {
                    required property var modelData
                    width: 110; height: 104
                    scale: tileArea.containsMouse ? 1.06 : 1.0
                    Behavior on scale { NumberAnimation { duration: 120 } }

                    Column {
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 8
                        AppIcon { size: 64; tint: modelData.tint; label: modelData.name; anchors.horizontalCenter: parent.horizontalCenter }
                        Text { text: modelData.name; color: Theme.text; font.pixelSize: 13; anchors.horizontalCenter: parent.horizontalCenter }
                    }
                    MouseArea {
                        id: tileArea; anchors.fill: parent; hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.launch(modelData.appId)
                    }
                }
            }
        }

        Text {
            visible: root.results.length === 0
            anchors.centerIn: parent
            text: "No apps match \"" + root.query + "\""
            color: Theme.textDim; font.pixelSize: 14
        }
    }
}
