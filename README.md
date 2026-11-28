# Aether Shell (prototype 0.1)

Desktop shell for Project Aether OS, written in Qt 6 / QML.
The shell is a source-level prototype. Some desktop functions now call host session services; Wi-Fi, Bluetooth, brightness and several system integrations remain demonstrations.

## What works
- Top bar with clock, status area, notification badge
- Dock with running-app indicators and hover animation
- Fullscreen launcher with live search; lists built-in apps plus installed Linux apps from XDG .desktop files, with theme icons (Enter launches the first match, Esc closes)
- Quick settings: demonstration Wi-Fi/Bluetooth toggles, dark mode, do not disturb, shell brightness dimming, audio output/microphone controls, and recent clipboard text
- Notification centre with toast popups and "Send test notification"
- Floating, draggable app windows (close, maximise, double-click title bar, focus/raise)
- Settings app: dark/light theme, accent colour, display, network demonstrations, audio devices and levels, Android runtime session controls, about
- Files app: home-folder browsing, folder creation, rename, trash, and opening files with registered desktop applications

## Session manager

The C++ `SessionManager` launches and tracks application processes requested by `ApplicationService`. It exposes a live list of process instances (desktop ID, application name, process ID, and starting/running state), reports start, exit, and launch-failure signals, and provides terminate-by-application and terminate-all requests. The existing launcher still calls `AppCatalog.launch()`; it does not need to know about process management.

This is a first session-level process tracker, not yet a complete desktop-session supervisor. It does not yet restore sessions, monitor child process trees, enforce resource limits, or coordinate logout and system shutdown.

## Session settings

The C++ `SessionSettings` service persists the appearance preferences used by the existing shell. Dark/light mode, accent colour, and do-not-disturb are stored through Qt's `QSettings` backend, loaded at startup, and exposed as typed properties to the shell. The Settings app and quick-settings dark-mode control use this service without changing the interface design. A schema version is recorded for future preference migrations.

This is an early persistence slice, not the complete settings system. Display brightness, Wi-Fi, Bluetooth, shell layout, startup preferences, and Android-specific preferences are not yet persisted or connected to real system services. Do-not-disturb is a user notification preference; it suppresses toast presentation but does not control an external notification daemon.

## Notification service

The C++ `NotificationService` owns the session notification list and exposes it to QML through the existing `Notifications` context property. It preserves the current `items`, `count`, `push(title, body)`, `clear()`, and `pushed` interfaces, so the notification centre and badge do not need a visual rewrite.

The service also exports the freedesktop `org.freedesktop.Notifications` interface on the session D-Bus when it can acquire that well-known name. It supports plain-text bodies, replacement IDs, close requests, expiry timers, and action buttons exposed in the notification centre. Action selections are returned to the originating application through the standard D-Bus `ActionInvoked` signal. Rich body markup is not advertised. Waydroid's host notification manager uses this standard interface to forward Android notifications, and native Linux applications can use it directly. If another notification daemon already owns the name, Aether records `name-in-use` and does not receive desktop notifications until the conflict is resolved.

## Audio service

The C++ `AudioService` exposes default output and input volume/mute state, enumerates PulseAudio-compatible sinks and sources with `pactl`, and selects default devices. Output and microphone level controls use `wpctl` with `pactl` fallbacks. Quick Settings and the Audio page in Settings use the same service. The service invokes these utilities with argument lists (not a shell command). Install `pipewire-bin` and/or `pulseaudio-utils` on Debian for the relevant controls. This is an initial host-control adapter: it does not yet provide per-application routing, audio stream management, hotplug notifications, audio capture APIs, or a validated Android audio bridge. Device-control commands are synchronous in this prototype and require review before production use.

## Clipboard service

The C++ `ClipboardService` uses the Qt session clipboard and keeps up to 30 recent non-empty text entries in memory for the current shell session. Quick Settings can restore an entry or clear the history. The history is not written to disk and is lost when the shell exits; clearing history does not erase the current system clipboard. This first version handles text only, not images, files, rich MIME data or clipboard persistence. No automatic Android clipboard synchronisation is claimed: cross-boundary sharing needs a dedicated, permission-aware Waydroid bridge and is still outstanding.

## File manager and file access

The Files app is connected to a C++ `FileService` model. It browses readable entries under the user's home directory, watches the current directory for changes, creates folders, renames entries, moves entries to the desktop trash through `gio trash`, and opens files through the registered desktop application. Operations are deliberately restricted to canonical paths inside the home directory; the current prototype does not expose arbitrary system paths. The file manager is an initial functional slice, not a complete desktop file system: removable-volume browsing, file picking for other apps, MIME/URL launch context, search, copy/move, archive handling, permissions, network shares, and Android shared-storage/document-provider integration remain outstanding. Install `libglib2.0-bin` for the `gio` trash command.

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

The Android provider has a concrete Waydroid adapter. When the `waydroid` executable is installed, it asynchronously requests the installed-app list and supports app launch, APK install/remove, and user-session start/stop operations. Launching an Android app automatically requests a user session first when one is not already marked running. Settings exposes session start/stop, refresh, and status controls. The launcher still uses the same unified `AppCatalog.launch(appId)` API. If Waydroid is absent, the provider reports itself unavailable and contributes no Android entries. The adapter does not install Waydroid, initialize an Android image, configure the host kernel, or itself implement Android service bridges. Notification forwarding is handled separately through Waydroid's host notification manager and Aether's freedesktop D-Bus endpoint when the installed Waydroid components support it. App listing depends on a live Waydroid session, and live behaviour has not yet been tested.

## Wayland window tracking and Android identity

`WaylandWindowTracker` opens a separate Wayland client connection and discovers `ext-foreign-toplevel-list-v1` at runtime. It reports mapped-window creation, metadata updates, and closure without attempting to control other applications' windows. This protocol does not report activation, so active/foreground state is not inferred.

The Waydroid adapter accepts explicit live-window-ID-to-package mappings through `registerWindowPackageMapping()`, and automatically resolves standard Waydroid multi-window app IDs of the form `waydroid.<package-name>`. It strips only that exact Waydroid namespace and validates the package against the installed Android catalogue; it does not infer identity from titles or arbitrary compositor app IDs. Since another local Wayland client can imitate an app ID, this association is for window grouping only and is not a security identity. This convention depends on Waydroid multi-window mode (`persist.waydroid.multi_windows=true`) being enabled in the Android image. The resolver is implemented in source but has not been validated against a running Waydroid image. Explicit mappings remain available for other trusted runtime bridges, and the mapping registry is in-memory. The wlr foreign-toplevel protocol fallback is not implemented yet.

Window tracking is optional at build time. If `libwayland-dev` and `libwayland-bin` are available, CMake generates the protocol bindings and links `wayland-client`. Otherwise the shell still builds without this feature and the tracker reports `unsupported-build`.

## Build (Ubuntu / Debian)
    sudo apt install build-essential cmake qt6-base-dev qt6-declarative-dev \
         qml6-module-qtquick qml6-module-qtquick-window
    # Optional host desktop utilities: pipewire-bin pulseaudio-utils libglib2.0-bin
    cmake -B build -S .
    cmake --build build -j
    ./build/aether-shell

Needs Qt 6.5 or newer (for loadFromModule). Optional foreign-toplevel tracking: install `libwayland-dev` and `libwayland-bin` before configuring CMake. On Wayland: `QT_QPA_PLATFORM=wayland ./build/aether-shell`.

## Layout
- `Main.qml` shell root and window management
- `Theme.qml`, `Apps.qml`, `System.qml` QML singletons (shared state)
- `SessionSettings.cpp/.h`: persistent user appearance preferences
- `NotificationService.cpp/.h` and `NotificationDBusAdaptor.cpp/.h`: notification model plus freedesktop D-Bus server
- `AudioService.cpp/.h`: PipeWire/PulseAudio-compatible device and level controls
- `ClipboardService.cpp/.h`: session clipboard and ephemeral text history
- `FileService.cpp/.h` and `FilesApp.qml`: home-folder file model and initial file manager
- `SessionManager.cpp/.h`: application process lifecycle tracking
- `ApplicationService.cpp/.h`: unified catalogue and provider-based launch dispatcher (Linux and optional Waydroid providers)
- `AndroidRuntimeAdapter.h`: Android runtime provider interface
- `WaydroidRuntimeAdapter.cpp/.h`: asynchronous Waydroid CLI adapter and explicit Android window identity resolver
- `WaylandWindowTracker.cpp/.h`: read-only foreign-toplevel observer
- `protocols/ext-foreign-toplevel-list-v1.xml`: vendored Wayland protocol definition
- `AppCatalog.cpp/.h`: themed icon image provider
- `TopBar`, `Dock`, `Launcher`, `QuickSettings`, `NotificationCenter`: shell components
- `AppWindow`, `SettingsApp`, `PlaceholderApp`: window frame and app content
- `System.qml` is still demonstration state; hardware/system integration is not implemented
