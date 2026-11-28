# Aether Shell (prototype 0.1)

A Qt 6 / QML desktop shell for Project Aether OS, intended to become the desktop environment of a Debian-based Linux operating system.

## Current functionality

- Top bar, dock, launcher, notification centre and floating application windows
- Installed application discovery through XDG desktop entries
- Theme and appearance controls
- Initial NetworkManager D-Bus integration for Wi-Fi radio state and access-point discovery
- Wi-Fi scanning from Quick Settings

The Wi-Fi connection flow (credential entry and saved connection profile activation) is not implemented yet. Bluetooth, hardware brightness, battery, audio and power controls remain prototype UI and are not represented as working hardware integrations.

## Build on Ubuntu / Debian

Install the build dependencies:

    sudo apt update
    sudo apt install build-essential cmake qt6-base-dev qt6-declarative-dev \
         qml6-module-qtquick qml6-module-qtquick-window network-manager

Build:

    cmake -B build -S .
    cmake --build build -j

Run:

    ./build/aether-shell

A working system D-Bus session and NetworkManager service are required for live Wi-Fi status and scanning. The user running Aether must have permission under the system's NetworkManager D-Bus policy to change wireless settings and request scans.

Qt 6.5 or newer is required. On Wayland:

    QT_QPA_PLATFORM=wayland ./build/aether-shell

## Architecture

- `main.cpp`: application entry point and QML context registration
- `SystemBridge.*`: C++ bridge exposed to QML
- `NetworkService.*`: NetworkManager D-Bus integration
- `System.qml`: shared system state; Wi-Fi values are read from the live bridge
- Remaining QML files: desktop shell and application UI

## Development status

This is an incremental desktop-shell project, not yet a complete operating system or installer. Hardware integrations are being added one service at a time. The next networking increment is secure connection activation using NetworkManager connection profiles; credentials must not be logged or stored by the shell itself.
