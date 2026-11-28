pragma Singleton
import QtQuick

// Live Linux system services exposed by the C++ bridge.
// Features not yet integrated remain clearly marked as prototype state.
QtObject {
    readonly property var network: SystemBridge.network

    readonly property bool wifi: network ? network.wifiEnabled : false
    property bool bluetooth: false
    property bool dnd: false
    property real brightness: 0.85
    property int battery: 0
}
