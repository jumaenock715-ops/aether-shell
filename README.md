# Aether Shell (prototype 0.1)

Desktop shell for Project Aether OS, written in Qt 6 / QML.
Controls are interface demonstrations; they are not yet connected to hardware services.

## What works
- Top bar with clock, status area, notification badge
- Dock with running-app indicators and hover animation
- Fullscreen launcher with live search; lists built-in apps plus installed apps read from XDG .desktop files, with theme icons (Enter launches the first match, Esc closes)
- Quick settings: Wi-Fi, Bluetooth, dark mode, do not disturb, brightness (dims the shell)
- Notification centre with toast popups and "Send test notification"
- Floating, draggable app windows (close, maximise, double-click title bar, focus/raise)
- Settings app: dark/light theme, accent colour, display, network, about

## Application service

The C++ `ApplicationService` discovers application desktop entries from XDG application directories and exposes metadata through the existing `AppCatalog` context property. The QML launcher retains its current interface.

The service currently:
- searches application directories recursively;
- derives desktop IDs from relative paths;
- honours higher-priority desktop-entry masking and basic visibility rules;
- checks `TryExec` and the launch executable;
- expands common desktop-entry launch field codes for launches without a file/URL context;
- launches with `QProcess` directly, without invoking a shell.

Terminal-based desktop entries and file/URL launch contexts are not yet supported. Full desktop-entry localization and specification conformance remain future work.

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
- `ApplicationService.cpp/.h`: session-level installed-application discovery and launch service
- `AppCatalog.cpp/.h`: themed icon image provider
- `TopBar`, `Dock`, `Launcher`, `QuickSettings`, `NotificationCenter`: shell components
- `AppWindow`, `SettingsApp`, `PlaceholderApp`: window frame and app content
- `System.qml` is still demonstration state; hardware/system integration is not implemented
