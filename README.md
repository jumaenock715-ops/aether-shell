# Aether Shell (prototype 0.1)

Desktop shell for Project Aether OS, written in Qt 6 / QML.
Controls are interface demonstrations; they are not yet connected to hardware services.

## What works
- Top bar with clock, status area, notification badge
- Dock with running-app indicators and hover animation
- Fullscreen launcher with live search (Enter launches the first match, Esc closes)
- Quick settings: Wi-Fi, Bluetooth, dark mode, do not disturb, brightness (dims the shell)
- Notification centre with toast popups and "Send test notification"
- Floating, draggable app windows (close, maximise, double-click title bar, focus/raise)
- Settings app: dark/light theme, accent colour, display, network, about

## Build (Ubuntu / Debian)
    sudo apt install build-essential cmake qt6-base-dev qt6-declarative-dev \
         qml6-module-qtquick qml6-module-qtquick-window
    cmake -B build -S .
    cmake --build build -j
    ./build/aether-shell

Needs Qt 6.5 or newer (for loadFromModule). On Wayland: `QT_QPA_PLATFORM=wayland ./build/aether-shell`.

## Layout
- `Main.qml` shell root and window management
- `Theme.qml`, `Apps.qml`, `System.qml`, `Notifications.qml` singletons (shared state)
- `TopBar`, `Dock`, `Launcher`, `QuickSettings`, `NotificationCenter` shell components
- `AppWindow`, `SettingsApp`, `PlaceholderApp` window frame and app content
- `System.qml` is the future bridge to real Aether system services (C++/Rust)
