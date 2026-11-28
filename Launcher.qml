import QtQuick
import QtQuick.Controls

Item {
    id: root
    signal launch(var app)
    signal closeRequested()

    property string query: ""
    readonly property var results: Apps.list.concat(AppCatalog.apps).filter(function (a) {
        return a.name.toLowerCase().indexOf(query.toLowerCase()) >= 0
    })

    onVisibleChanged: if (visible) { query = ""; search.text = ""; search.forceActiveFocus() }

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, Theme.dark ? 0.48 : 0.22)
        MouseArea { anchors.fill: parent; onClicked: root.closeRequested() }
    }

    Rectangle {
        id: panel
        anchors.centerIn: parent
        width: Math.min(720, root.width - 40)
        height: Math.min(590, root.height - 100)
        radius: 28
        color: Theme.launcherGlass
        border.color: Theme.border
        border.width: 1

        Column {
            anchors.fill: parent
            anchors.margins: 26
            spacing: 18

            Item {
                width: parent.width; height: 42
                Rectangle {
                    x: 0; y: 0; width: 42; height: 42; radius: 14
                    color: Theme.accent
                    Text { anchors.centerIn: parent; text: "A"; color: "white"; font.pixelSize: 23; font.bold: true }
                }
                Column {
                    x: 54; anchors.verticalCenter: parent.verticalCenter
                    spacing: 2
                    Text { text: "Aether"; color: Theme.text; font.pixelSize: 20; font.bold: true }
                    Text { text: "Your workspace"; color: Theme.textDim; font.pixelSize: 11 }
                }
                Text {
                    anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                    text: "×"; color: Theme.textDim; font.pixelSize: 22
                    MouseArea { anchors.fill: parent; anchors.margins: -8; cursorShape: Qt.PointingHandCursor; onClicked: root.closeRequested() }
                }
            }

            Rectangle {
                id: searchBox
                width: parent.width
                height: 48; radius: 15
                color: Theme.surface
                border.color: search.activeFocus ? Theme.accent : Theme.border
                TextInput {
                    id: search
                    anchors.fill: parent
                    anchors.leftMargin: 17; anchors.rightMargin: 17
                    verticalAlignment: TextInput.AlignVCenter
                    color: Theme.text; font.pixelSize: 14
                    selectionColor: Theme.accent
                    clip: true
                    onTextChanged: root.query = text
                    Keys.onEscapePressed: root.closeRequested()
                    Keys.onReturnPressed: if (root.results.length > 0) root.launch(root.results[0])
                    Text {
                        visible: search.text.length === 0
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Search apps and settings"
                        color: Theme.textDim; font.pixelSize: 14
                    }
                }
            }

            Row {
                width: parent.width
                Text { text: root.query.length ? "Search results" : "Pinned apps"; color: Theme.text; font.pixelSize: 14; font.bold: true }
                Item { width: 1; height: 1 }
                Text { text: root.results.length + " apps"; color: Theme.textDim; font.pixelSize: 11 }
            }

            Flickable {
                id: scroller
                width: parent.width
                height: Math.max(120, parent.height - 210)
                clip: true
                contentWidth: width
                contentHeight: grid.height
                boundsBehavior: Flickable.StopAtBounds

                Flow {
                    id: grid
                    width: scroller.width
                    spacing: 12
                    Repeater {
                        model: root.results
                        delegate: Item {
                            required property var modelData
                            width: 104; height: 112
                            scale: tileArea.containsMouse ? 1.045 : 1.0
                            Behavior on scale { NumberAnimation { duration: 120 } }

                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: 2
                                radius: 16
                                color: tileArea.containsMouse ? Theme.surface : "transparent"
                                border.color: tileArea.containsMouse ? Theme.border : "transparent"
                            }
                            Column {
                                anchors.centerIn: parent
                                spacing: 8
                                AppIcon {
                                    size: 48
                                    tint: modelData.tint
                                    label: modelData.name
                                    iconName: modelData.icon !== undefined ? modelData.icon : ""
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                                Text {
                                    width: 94
                                    text: modelData.name
                                    elide: Text.ElideRight
                                    horizontalAlignment: Text.AlignHCenter
                                    color: Theme.text; font.pixelSize: 11
                                }
                            }
                            MouseArea {
                                id: tileArea; anchors.fill: parent; hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.launch(modelData)
                            }
                        }
                    }
                }
            }

            Text {
                visible: root.results.length === 0
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: "No apps match \"" + root.query + "\""
                color: Theme.textDim; font.pixelSize: 13
            }
        }
    }
}
