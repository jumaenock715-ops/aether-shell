import QtQuick
import QtQuick.Dialogs

Item {
    id: root
    property string newFolderName: ""
    property string renameOldName: ""

    FileDialog {
        id: openPicker
        title: "Open a file"
        fileMode: FileDialog.OpenFile
        onAccepted: FileService.openSelectedFile(selectedFile.toLocalFile())
    }

    Column {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        Row {
            width: parent.width
            spacing: 8
            Rectangle {
                width: 48; height: 32; radius: 7; color: Theme.surface; border.color: Theme.border
                Text { anchors.centerIn: parent; text: "↑ Up"; color: Theme.text; font.pixelSize: 12 }
                MouseArea { anchors.fill: parent; onClicked: FileService.goUp() }
            }
            Text {
                width: parent.width - 60; height: 32
                verticalAlignment: Text.AlignVCenter
                text: FileService.currentPath
                elide: Text.ElideMiddle
                color: Theme.textDim; font.pixelSize: 11
            }
        }

        Row {
            width: parent.width
            spacing: 8
            Rectangle {
                width: 90; height: 30; radius: 7; color: Theme.accent
                Text { anchors.centerIn: parent; text: "New folder"; color: "white"; font.pixelSize: 11 }
                MouseArea { anchors.fill: parent; onClicked: folderEntry.visible = !folderEntry.visible }
            }
            Rectangle {
                width: 72; height: 30; radius: 7; color: Theme.surface; border.color: Theme.border
                Text { anchors.centerIn: parent; text: "Refresh"; color: Theme.text; font.pixelSize: 11 }
                MouseArea { anchors.fill: parent; onClicked: FileService.refresh() }
            }
            Rectangle {
                width: 82; height: 30; radius: 7; color: Theme.surface; border.color: Theme.border
                Text { anchors.centerIn: parent; text: "Open file…"; color: Theme.text; font.pixelSize: 11 }
                MouseArea { anchors.fill: parent; onClicked: openPicker.open() }
            }
        }

        Column {
            width: parent.width
            spacing: 5
            Text { text: "Removable storage"; color: Theme.textDim; font.pixelSize: 10 }
            Row {
                width: parent.width
                spacing: 6
                Repeater {
                    model: FileService.volumes
                    delegate: Rectangle {
                        required property var modelData
                        width: Math.min(150, parent.width / 2 - 4); height: 28
                        radius: 7; color: Theme.surface; border.color: Theme.border
                        Text {
                            anchors.fill: parent; anchors.margins: 6
                            text: modelData.name; color: Theme.text; font.pixelSize: 10
                            elide: Text.ElideMiddle; verticalAlignment: Text.AlignVCenter
                        }
                        MouseArea { anchors.fill: parent; onClicked: FileService.openDirectory(modelData.path) }
                    }
                }
                Rectangle {
                    visible: FileService.androidSharedPath.length > 0
                    width: 150; height: 28; radius: 7
                    color: Theme.surface; border.color: Theme.border
                    Text { anchors.centerIn: parent; text: "Android shared storage"; color: Theme.text; font.pixelSize: 10 }
                    MouseArea { anchors.fill: parent; onClicked: FileService.openDirectory(FileService.androidSharedPath) }
                }
            }
        }

        Rectangle {
            id: folderEntry
            visible: false
            width: parent.width; height: 38
            color: Theme.surface; radius: 7; border.color: Theme.border
            Row {
                anchors.fill: parent; anchors.margins: 5; spacing: 6
                TextInput {
                    id: folderName
                    width: parent.width - 64; height: parent.height
                    verticalAlignment: TextInput.AlignVCenter
                    color: Theme.text; font.pixelSize: 12
                    clip: true
                    Text { visible: folderName.text.length === 0; text: "Folder name"; color: Theme.textDim; anchors.verticalCenter: parent.verticalCenter }
                    Keys.onReturnPressed: {
                        if (FileService.createDirectory(folderName.text)) {
                            folderName.text = ""; folderEntry.visible = false
                        }
                    }
                }
                Text {
                    text: "Create"; color: Theme.accent2; font.pixelSize: 12
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (FileService.createDirectory(folderName.text)) {
                                folderName.text = ""; folderEntry.visible = false
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            visible: root.renameOldName.length > 0
            width: parent.width; height: 36; radius: 7
            color: Theme.surface; border.color: Theme.border
            Row {
                anchors.fill: parent; anchors.margins: 5; spacing: 6
                TextInput {
                    id: renameInput
                    width: parent.width - 64; height: parent.height
                    text: root.renameOldName
                    verticalAlignment: TextInput.AlignVCenter
                    color: Theme.text; font.pixelSize: 12
                }
                Text {
                    text: "Save"; color: Theme.accent2; font.pixelSize: 12
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (FileService.renameEntry(root.renameOldName, renameInput.text))
                                root.renameOldName = ""
                        }
                    }
                }
            }
        }

        ListView {
            id: entries
            width: parent.width
            height: parent.height - 100
            clip: true
            spacing: 3
            model: FileService
            delegate: Rectangle {
                required property string name
                required property string path
                required property bool isDirectory
                required property string size
                required property string modified
                width: entries.width; height: 44; radius: 7
                color: itemArea.containsMouse ? Theme.surface : "transparent"
                Row {
                    z: 1
                    anchors.fill: parent; anchors.leftMargin: 9; anchors.rightMargin: 9
                    spacing: 10
                    Text {
                        width: 24; height: parent.height
                        verticalAlignment: Text.AlignVCenter
                        text: isDirectory ? "▰" : "▤"
                        color: isDirectory ? Theme.accent2 : Theme.textDim
                        font.pixelSize: 16
                    }
                    Column {
                        width: parent.width - 200; height: parent.height
                        spacing: 1
                        Text { width: parent.width; text: name; color: Theme.text; font.pixelSize: 12; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter }
                        Text { width: parent.width; text: isDirectory ? "Folder" : size + " bytes"; color: Theme.textDim; font.pixelSize: 10 }
                    }
                    Text {
                        width: 62; height: parent.height
                        text: modified; color: Theme.textDim; font.pixelSize: 9
                        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                    }
                    Text {
                        text: "Rename"; color: Theme.accent2; font.pixelSize: 9
                        MouseArea {
                            anchors.fill: parent
                            onClicked: { root.renameOldName = name; renameInput.text = name }
                        }
                    }
                    Text {
                        text: "Trash"; color: "#FF5C7A"; font.pixelSize: 9
                        MouseArea { anchors.fill: parent; onClicked: FileService.trashEntry(path) }
                    }
                }
                MouseArea {
                    id: itemArea
                    z: 0
                    anchors.fill: parent; hoverEnabled: true
                    onDoubleClicked: {
                        if (isDirectory) FileService.openDirectory(path)
                        else FileService.openFile(path)
                    }
                }
            }
            Text {
                visible: FileService.count === 0
                anchors.centerIn: parent
                text: "This folder is empty"
                color: Theme.textDim; font.pixelSize: 12
            }
        }
        Text {
            width: parent.width
            text: "Browse your home folder, attached removable drives and detected Waydroid shared storage. Double-click files to open them with their registered MIME applications."
            wrapMode: Text.WordWrap
            color: Theme.textDim; font.pixelSize: 10
        }
    }
}
