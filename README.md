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

## Session manager

The C++ `SessionManager` launches and tracks application processes requested by `ApplicationService`. It exposes a live list of process instances (desktop ID, application name, process ID, and starting/running state), reports start, exit, and launch-failure signals, and provides terminate-by-application and terminate-all requests. The existing launcher still calls `AppCatalog.launch()`; it does not need to know about process management.

This is a first session-level process tracker, not yet a complete desktop-session supervisor. It does not yet restore sessions, monitor child process trees, enforce resource limits, or coordinate logout and system shutdown.

## Session settings

The C++ `SessionSettings` service persists the appearance preferences used by the existing shell. Dark/light mode and accent colour are stored through Qt's `QSettings` backend, loaded at startup, and exposed as typed properties to the existing `Theme` singleton. The Settings app and quick-settings dark-mode control use this service without changing the interface design. A schema version is recorded for future preference migrations.

This is the first persistence slice, not the complete settings system. Display brightness, Wi-Fi, Bluetooth, do-not-disturb, shell layout, startup preferences, and Android-specific settings are not yet persisted or connected to real system services.

## Notification service

The C++ `NotificationService` owns the session notification list and exposes it to QML through the existing `Notifications` context property. It preserves the current `items`, `count`, `push(title, body)`, `clear()`, and `pushed` interfaces, so the notification centre and badge do not need a visual rewrite.

This is currently an in-process notification model. It does not yet receive notifications from other applications or publish them through the Linux desktop notification D-Bus interface.

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
- `Theme.qml`, `Apps.qml`, `System.qml` QML singletons (shared state)
- `SessionSettings.cpp/.h`: persistent user appearance preferences
- `NotificationService.cpp/.h`: session notification model exposed to QML
- `SessionManager.cpp/.h`: application process lifecycle tracking
- `ApplicationService.cpp/.h`: session-level installed-application discovery and launch service
- `AppCatalog.cpp/.h`: themed icon image provider
- `TopBar`, `Dock`, `Launcher`, `QuickSettings`, `NotificationCenter`: shell components
- `AppWindow`, `SettingsApp`, `PlaceholderApp`: window frame and app content
- `System.qml` is still demonstration state; hardware/system integration is not implemented
