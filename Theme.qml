pragma Singleton
import QtQuick

QtObject {
    property bool dark: true
    property color accent: "#6D4AFF"
    readonly property color accent2: "#14B8D4"

    readonly property color bg: dark ? "#0B1020" : "#E9ECF8"
    readonly property color surface: dark ? Qt.rgba(1, 1, 1, 0.10) : Qt.rgba(1, 1, 1, 0.70)
    readonly property color surfaceStrong: dark ? "#161C33" : "#FFFFFF"
    readonly property color panel: dark ? Qt.rgba(0.09, 0.11, 0.2, 0.92) : Qt.rgba(1, 1, 1, 0.92)
    readonly property color border: dark ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(0, 0, 0, 0.10)
    readonly property color text: dark ? "#F4F5FB" : "#161B2E"
    readonly property color textDim: dark ? "#9AA3BC" : "#5B6478"

    readonly property int radius: 14
    readonly property int animMs: 180
    readonly property int barHeight: 36
}
