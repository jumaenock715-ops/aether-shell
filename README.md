# Aether Shell (prototype 0.1)

Desktop shell for Project Aether OS, written in Qt 6 / QML.
Controls are interface demonstrations; they are not yet connected to hardware services.

## What works
- Top bar with clock, status area, notification badge
- Dock with running-app indicators and hover animation
- Fullscreen launcher with live search; lists built-in apps plus installed Linux apps from XDG .desktop files, with theme icons (Enter launches the first match, Esc closes)
- Quick settings: Wi-Fi, Bluetooth, dark mode, do not disturb, brightness (dims the shell)
- Notification centre with toast popups and "Send test notification"
- Floating, draggable app windows (close, maximise, double-click title bar, focus/raise)
- Settings app: dark/light theme, accent colour, display, network, Android runtime session controls, about

## Session manager

The C++ `SessionManager` launches and tracks application processes requested by `ApplicationService`. It exposes a live list of process instances (desktop ID, application name, process ID, and starting/running state), reports start, exit, and launch-failure signals, and provides terminate-by-application and terminate-all requests. The existing launcher still calls `AppCatalog.launch()`; it does not need to know about process management.

This is a first session-level process tracker, not yet a complete desktop-session supervisor. It does not yet restore sessions, monitor child process trees, enforce resource limits, or coordinate logout and system shutdown.

## Session settings

The C++ `SessionSettings` service persists the appearance preferences used by the existing shell. Dark/light mode, accent colour, and do-not-disturb are stored through Qt's `QSettings` backend, loaded at startup, and exposed as typed properties to the shell. The Settings app and quick-settings dark-mode control use this service without changing the interface design. A schema version is recorded for future preference migrations.

This is an early persistence slice, not the complete settings system. Display brightness, Wi-Fi, Bluetooth, shell layout, startup preferences, and Android-specific preferences are not yet persisted or connected to real system services. Do-not-disturb is a user notification preference; it suppresses toast presentation but does not control an external notification daemon.

## Notification service

The C++ `NotificationService` owns the session notification list and exposes it to QML through the existing `Notifications` context property. It preserves the current `items`, `count`, `push(title, body)`, `clear()`, and `pushed` interfaces, so the notification centre and badge do not need a visual rewrite.

This is currently an in-process notification model. It does not yet receive notifications from other applications or publish them through the Linux desktop notification D-Bus interface.

## Application service

The C++ `ApplicationService` exposes one application catalogue through the existing `AppCatalog` context property. Linux desktop entries and Android packages use stable provider-prefixed IDs (`linux:` and `android:`). The launcher routes both provider types through the same `AppCatalog.launch(appId)` call and retains its current visual interface. `AndroidRuntimeAdapter.h` defines the provider boundary for runtime discovery, app listing, launch, APK installation, and package removal.

The Linux provider currently:
- searches application directories recursively;
- derives desktop IDs from relative paths;
- honours higher-priority desktop-entry masking and basic visibility rules;
- checks `TryExec` and the launch executable;
- expands common desktop-entry launch field codes for launches without a file/URL context;
- launches with `QProcess` directly, without invoking a shell.

Terminal-based desktop entries and file/URL launch contexts are not yet supported. Full desktop-entry localization and specification conformance remain future work.

The Android provider has a concrete Waydroid adapter. When the `waydroid` executable is installed, it asynchronously requests the installed-app list and supports app launch, APK install/remove, and user-session start/stop operations. Launching an Android app automatically requests a user session first when one is not already marked running. Settings exposes session start/stop, refresh, and status controls. The launcher still uses the same unified `AppCatalog.launch(appId)` API. If Waydroid is absent, the provider reports itself unavailable and contributes no Android entries. The adapter does not install Waydroid, initialize an Android image, configure the host kernel, or provide Android notification/service bridges. App listing depends on a live Waydroid session, and live behaviour has not yet been tested.

## Wayland window tracking and Android identity

`WaylandWindowTracker` opens a separate Wayland client connection and discovers `ext-foreign-toplevel-list-v1` at runtime. It reports mapped-window creation, metadata updates, and closure without attempting to control other applications' windows. This protocol does not report activation, so active/foreground state is not inferred.

The Waydroid adapter accepts explicit live-window-ID-to-package mappings through `registerWindowPackageMapping()`. A mapping is accepted only for a currently observed window and an installed Android package. Unmapped windows are reported as unassociated; window titles and compositor app IDs are never used to guess package identity. The mapping registry is currently in-memory and awaits a trusted Waydroid/Android surface-metadata source. The wlr foreign-toplevel protocol fallback is not implemented yet.

Window tracking is optional at build time. If `libwayland-dev` and `libwayland-bin` are available, CMake generates the protocol bindings and links `wayland-client`. Otherwise the shell still builds without this feature and the tracker reports `unsupported-build`.

## Build (Ubuntu / Debian)
    sudo apt install build-essential cmake qt6-base-dev qt6-declarative-dev \
         qml6-module-qtquick qml6-module-qtquick-window
    cmake -B build -S .
    cmake --build build -j
    ./build/aether-shell

Needs Qt 6.5 or newer (for loadFromModule). Optional foreign-toplevel tracking: install `libwayland-dev` and `libwayland-bin` before configuring CMake. On Wayland: `QT_QPA_PLATFORM=wayland ./build/aether-shell`.

## Layout
- `Main.qml` shell root and window management
- `Theme.qml`, `Apps.qml`, `System.qml` QML singletons (shared state)
- `SessionSettings.cpp/.h`: persistent user appearance preferences
- `NotificationService.cpp/.h`: session notification model exposed to QML
- `SessionManager.cpp/.h`: application process lifecycle tracking
- `ApplicationService.cpp/.h`: unified catalogue and provider-based launch dispatcher (Linux and optional Waydroid providers)
- `AndroidRuntimeAdapter.h`: Android runtime provider interface
- `WaydroidRuntimeAdapter.cpp/.h`: asynchronous Waydroid CLI adapter and explicit Android window identity resolver
- `WaylandWindowTracker.cpp/.h`: read-only foreign-toplevel observer\n- `protocols/ext-foreign-toplevel-list-v1.xml`: vendored Wayland protocol definition
- `AppCatalog.cpp/.h`: themed icon image provider
- `TopBar`, `Dock`, `Launcher`, `QuickSettings`, `NotificationCenter`: shell components
- `AppWindow`, `SettingsApp`, `PlaceholderApp`: window frame and app content
- `System.qml` is still demonstration state; hardware/system integration is not implemented
