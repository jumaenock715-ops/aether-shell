# Aether Shell (prototype 0.1)

Desktop shell for Project Aether OS, written in Qt 6 / QML.
The shell is a source-level prototype. Some desktop functions now call host session services; Wi-Fi, Bluetooth, brightness and several system integrations remain demonstrations.

## What works
- Top bar with clock, status area, notification badge
- Centred bottom taskbar with Aether launcher, pinned applications, running indicators and hover animation
- Centred floating application launcher with live search, app grid and theme icons; lists built-in apps plus installed Linux apps from XDG .desktop files (Enter launches the first match, Esc closes)
- Quick settings: demonstration Wi-Fi/Bluetooth toggles, dark mode, do not disturb, shell brightness dimming, audio output/microphone controls, and rich recent clipboard history (text, images and file URLs)
- Notification centre with toast popups and "Send test notification"
- Floating, draggable app windows (close, maximise, double-click title bar, focus/raise)
- Settings app: dark/light theme, accent colour, display, network demonstrations, D-Bus notification status, audio device selection and per-stream mixer/routing, Android runtime and bridge status, about
- Files app: home and removable-volume browsing, detected Waydroid shared storage, folder creation, rename, trash, file picker and MIME/default-app launching

## Session manager

The C++ `SessionManager` launches and tracks application processes requested by `ApplicationService`. It exposes a live list of process instances (desktop ID, application name, process ID, and starting/running state), reports start, exit, and launch-failure signals, and provides terminate-by-application and terminate-all requests. The existing launcher still calls `AppCatalog.launch()`; it does not need to know about process management.

This is a first session-level process tracker, not yet a complete desktop-session supervisor. It does not yet restore sessions, monitor child process trees, enforce resource limits, or coordinate logout and system shutdown.

## Session settings

The C++ `SessionSettings` service persists the appearance preferences used by the existing shell. Dark/light mode, accent colour, and do-not-disturb are stored through Qt's `QSettings` backend, loaded at startup, and exposed as typed properties to the shell. The Settings app and quick-settings dark-mode control use this service without changing the interface design. A schema version is recorded for future preference migrations.

This is an early persistence slice, not the complete settings system. Display brightness, Wi-Fi, Bluetooth, shell layout, startup preferences, and Android-specific preferences are not yet persisted or connected to real system services. Do-not-disturb is a user notification preference; it suppresses toast presentation but does not control an external notification daemon.

## Notification service

The C++ `NotificationService` owns the session notification list and exposes it to QML through the existing `Notifications` context property. It preserves the current `items`, `count`, `push(title, body)`, `clear()`, and `pushed` interfaces, so the notification centre and badge do not need a visual rewrite.

The service exports the freedesktop `org.freedesktop.Notifications` interface on the session D-Bus when it owns the well-known name. It supports plain-text bodies, replacement IDs, close requests, expiry timers and action buttons, and emits the standard `NotificationClosed` and `ActionInvoked` signals. Rich body markup is not advertised. If another notification daemon already owns the name, Aether keeps its internal notification centre working, reports `name-in-use`, watches for the incumbent owner to exit, and retries registration. The optional Aether Android Bridge companion forwards Android notification post/removal events from Waydroid shared storage into this same model after the user grants Android notification access. Live D-Bus interoperability and Android delivery still require runtime validation.

## Audio service

The C++ `AudioService` exposes default output/input volume and mute, enumerates sinks/sources, selects default devices, and discovers active playback streams through the PulseAudio-compatible `pactl` interface. Settings can change each stream's volume and mute state and move it between output devices. Device changes are observed through `pactl subscribe`, with a periodic refresh fallback. Waydroid audio is controllable as a host stream when the Android runtime exposes it through PipeWire/PulseAudio; the runtime may aggregate several Android apps into one stream, so per-Android-app isolation is not guaranteed. Audio capture APIs and end-to-end Android audio validation remain outstanding. Install `pipewire-bin` and/or `pulseaudio-utils` on Debian. These utility calls are synchronous in this prototype and should be moved off the UI thread before production.

## Clipboard service

The C++ `ClipboardService` uses the Qt session clipboard and keeps up to 30 recent entries in memory. It captures and restores plain text, image data and file/URL lists; image previews are cached under the user's application cache directory for the current session. History is not persisted, and clearing it does not erase the current clipboard. The Aether Android Bridge companion synchronises plain text through Waydroid app-specific shared storage while its activity is foregrounded. Android background clipboard restrictions mean this is not an always-on clipboard bridge; image/file clipboard transfer across the Android boundary remains unsupported.

## File manager and file access

The Files app is connected to a C++ `FileService` model. It browses the home directory and detected removable mount points, watches the current directory, creates folders, renames entries, moves eligible entries to the desktop trash through `gio trash`, and opens files through their registered MIME/default applications. The built-in Files window includes a Qt Quick file picker. A detected Waydroid media root is exposed as an additional browse location, including the companion bridge's app-specific files. Access is constrained to canonical paths under home, detected removable mounts or the detected Waydroid media root; arbitrary system paths remain excluded. A system-wide XDG FileChooser portal backend, URI/file launch context for arbitrary Linux apps, search, copy/move, archive handling, permissions, network shares and Android Storage Access Framework/document-provider integration remain future work. Install `libglib2.0-bin` for the `gio` trash command.

## Android companion bridge

The optional Android companion lives in `android-bridge/`. Build its APK with Gradle 8.7 and Android SDK 35, install it inside Waydroid, open **Aether Bridge**, and grant notification access. Android notifications are appended as events in the app's own external-files directory; the host shell polls that directory and forwards events to the Aether notification centre. Text clipboard synchronisation is active only while the companion activity is open. The host bridge deliberately does not request elevated privileges or mount storage itself.

The host discovers the Waydroid media root at `~/.local/share/waydroid/data/media/0` or `/var/lib/waydroid/data/media/0`. If the directory is inaccessible to the host user, the bridge will remain unavailable; correct Waydroid storage permissions/mount setup are required. Notification access is user-granted, and clipboard synchronisation is subject to Android foreground clipboard restrictions. The bridge is source-implemented but has not yet been validated against a live Waydroid installation.

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

The Android provider has a concrete Waydroid adapter. When the `waydroid` executable is installed, it asynchronously requests the installed-app list and supports app launch, APK install/remove, and user-session start/stop operations. Launching an Android app automatically requests a user session first when one is not already marked running. The optional `android-bridge/` companion adds notification forwarding and foreground-only plain-text clipboard synchronisation through Waydroid's app-specific external storage. The bridge APK must be built, installed inside Waydroid, and granted notification access. The host bridge detects its shared-storage directory and imports notification events into Aether's notification model. The companion does not yet forward notification actions/replies or rich notification content. No Android image/kernel configuration is performed automatically, and live runtime delivery still needs validation.

## Wayland window tracking and Android identity

`WaylandWindowTracker` opens a separate Wayland client connection and discovers `ext-foreign-toplevel-list-v1` at runtime. It reports mapped-window creation, metadata updates, and closure without attempting to control other applications' windows. This protocol does not report activation, so active/foreground state is not inferred.

The Waydroid adapter accepts explicit live-window-ID-to-package mappings through `registerWindowPackageMapping()`, and automatically resolves standard Waydroid multi-window app IDs of the form `waydroid.<package-name>`. It strips only that exact Waydroid namespace and validates the package against the installed Android catalogue; it does not infer identity from titles or arbitrary compositor app IDs. Since another local Wayland client can imitate an app ID, this association is for window grouping only and is not a security identity. This convention depends on Waydroid multi-window mode (`persist.waydroid.multi_windows=true`) being enabled in the Android image. The resolver is implemented in source but has not been validated against a running Waydroid image. Explicit mappings remain available for other trusted runtime bridges, and the mapping registry is in-memory. The wlr foreign-toplevel protocol fallback is not implemented yet.

Window tracking is optional at build time. If `libwayland-dev` and `libwayland-bin` are available, CMake generates the protocol bindings and links `wayland-client`. Otherwise the shell still builds without this feature and the tracker reports `unsupported-build`.

## Build (Ubuntu / Debian)
    sudo apt install build-essential cmake qt6-base-dev qt6-declarative-dev \
         qml6-module-qtquick qml6-module-qtquick-window qml6-module-qtquick-dialogs
    # Optional host desktop utilities (audio + trash):
    sudo apt install pipewire-bin pulseaudio-utils libglib2.0-bin
    cmake -B build -S .
    cmake --build build -j
    ./build/aether-shell

Needs Qt 6.5 or newer (for loadFromModule). Optional foreign-toplevel tracking: install `libwayland-dev` and `libwayland-bin` before configuring CMake. On Wayland: `QT_QPA_PLATFORM=wayland ./build/aether-shell`.

## Layout
- `Main.qml` shell root and window management
- `Theme.qml`, `Apps.qml`, `System.qml` QML singletons (shared state)
- `SessionSettings.cpp/.h`: persistent user appearance preferences
- `NotificationService.cpp/.h` and `NotificationDBusAdaptor.cpp/.h`: notification model plus freedesktop D-Bus server
- `AudioService.cpp/.h`: PipeWire/PulseAudio-compatible device, per-stream mixer/routing and hotplug monitor
- `ClipboardService.cpp/.h`: session clipboard with text, image and file-URL history
- `FileService.cpp/.h` and `FilesApp.qml`: home/removable/Waydroid shared-storage model and file manager
- `SessionManager.cpp/.h`: application process lifecycle tracking
- `ApplicationService.cpp/.h`: unified catalogue and provider-based launch dispatcher (Linux and optional Waydroid providers)
- `AndroidRuntimeAdapter.h`: Android runtime provider interface
- `WaydroidRuntimeAdapter.cpp/.h`: asynchronous Waydroid CLI adapter and explicit Android window identity resolver
- `WaydroidBridgeService.cpp/.h`: host-side Android notification and text-clipboard bridge
- `android-bridge/`: optional Android companion app; built by the separate Android bridge workflow
- `WaylandWindowTracker.cpp/.h`: read-only foreign-toplevel observer
- `protocols/ext-foreign-toplevel-list-v1.xml`: vendored Wayland protocol definition
- `AppCatalog.cpp/.h`: themed icon image provider
- `TopBar`, `Dock`, `Launcher`, `QuickSettings`, `NotificationCenter`: shell components
- `AppWindow`, `SettingsApp`, `FilesApp`, `PlaceholderApp`: window frame and app content
- `System.qml` is still demonstration state; hardware/system integration is not implemented
