pragma Singleton
import QtQuick

// Live Linux system services exposed by the C++ bridge.
// Features not yet integrated remain clearly marked as prototype state.
QtObject {
    readonly property var network: SystemBridge.network

    readonly property bool wifi: network ? network.wifiEnabled : false
    readonly property var hardware: SystemBridge.hardware
    readonly property bool bluetooth: hardware ? hardware.bluetoothPowered : false
    property bool dnd: false
    readonly property real brightness: hardware && hardware.brightness >= 0 ? hardware.brightness / 100 : 0
    readonly property int battery: hardware ? hardware.batteryPercent : 0
    readonly property string batteryStatus: hardware ? hardware.batteryStatus : "Unknown"
    readonly property int batteryHealth: hardware ? hardware.batteryHealth : -1
}
