pragma Singleton
import QtQuick

// Demo state only. Later this becomes the bridge to real Aether system services.
QtObject {
    property bool wifi: true
    property bool bluetooth: true
    property bool dnd: false
    property real brightness: 0.85
    property int battery: 82
}
