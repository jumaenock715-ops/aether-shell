pragma Singleton
import QtQuick

QtObject {
    readonly property bool dark: SessionSettings.darkMode
    readonly property color accent: SessionSettings.accentColor
    readonly property color accent2: "#14B8D4"

    readonly property color bg: dark ? "#111522" : "#E9EDF5"
    readonly property color surface: dark ? Qt.rgba(0.12, 0.15, 0.23, 0.94) : Qt.rgba(1, 1, 1, 0.82)
    readonly property color surfaceStrong: dark ? "#1B2130" : "#FFFFFF"
    readonly property color panel: dark ? Qt.rgba(0.10, 0.13, 0.21, 0.96) : Qt.rgba(0.97, 0.98, 1.0, 0.97)
    readonly property color launcherPanel: dark ? "#202738" : "#F7F8FC"
    readonly property color taskbar: dark ? Qt.rgba(0.10, 0.13, 0.20, 0.94) : Qt.rgba(0.97, 0.98, 1.0, 0.91)
    readonly property color border: dark ? Qt.rgba(1, 1, 1, 0.13) : Qt.rgba(0.12, 0.16, 0.24, 0.10)
    readonly property color text: dark ? "#F4F6FB" : "#1B2332"
    readonly property color textDim: dark ? "#A5AEC1" : "#667085"

    readonly property int radius: 18
    readonly property int animMs: 190
    readonly property int barHeight: 34
}
