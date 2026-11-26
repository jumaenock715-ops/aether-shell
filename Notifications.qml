pragma Singleton
import QtQuick

QtObject {
    id: root
    property ListModel items: ListModel {}
    readonly property int count: items.count
    signal pushed(string title, string body)

    function push(title, body) {
        items.insert(0, { title: title, body: body,
                          time: Qt.formatTime(new Date(), "hh:mm") })
        pushed(title, body)
    }
    function clear() { items.clear() }
}
