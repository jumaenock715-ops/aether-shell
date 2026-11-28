# Aether Shell (prototype 0.1)

A Qt 6 / QML desktop shell for Project Aether OS, intended to become the desktop environment of a Debian-based Linux operating system.

## Current functionality

- Top bar, dock, launcher, notification centre and floating application windows
- Installed application discovery through XDG desktop entries
- Theme and appearance controls
- Initial NetworkManager D-Bus integration for Wi-Fi radio state and access-point discovery
- Wi-Fi scanning from Quick Settings

Hardware controls call the host Linux services/utilities and therefore require those components to be installed and permitted. Wi-Fi credentials are sent directly to NetworkManager over the system D-Bus and are not logged by Aether. Bluetooth device commands use BlueZ bluetoothctl. Audio requires PipeWire/wpctl; backlight adjustment requires brightnessctl and suitable device permissions; power actions require systemd/polkit authorization. CI verifies compilation only; hardware behaviour still requires testing on a physical Debian/Ubuntu machine.

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
- `NetworkService.*`: NetworkManager D-Bus integration\n- `LinuxHardwareService.*`: BlueZ, PipeWire, backlight, battery and systemd integration
- `System.qml`: shared system state; Wi-Fi values are read from the live bridge
- Remaining QML files: desktop shell and application UI

## Development status

This is an incremental desktop-shell project, not yet a complete operating system or installer. Hardware integrations are being added one service at a time. The next networking increment is secure connection activation using NetworkManager connection profiles; credentials must not be logged or stored by the shell itself.
