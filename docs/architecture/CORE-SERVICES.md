# Aether OS Core Services — Architecture Foundation

**Status:** Initial session services implemented; service integration and persistence remain in progress.  
**Target base:** Debian initially; Yocto integration remains preserved separately.  
**Shell:** Existing Qt 6 / QML Aether Shell.  
**Primary application workload:** Android applications, with native Linux applications also supported.

## Purpose and development mode

This document defines the boundary between Aether Shell and the operating-system services it will use. Current work is source and architecture development. Debian installation, image creation, booting, and runtime testing remain postponed until the source foundation is sufficiently complete.

The existing shell, its visual design, QML components, and assets must be preserved. Service work should connect to the existing interface rather than replace it with a new UI.

## Architectural principles

1. QML presents state and requests actions. It does not directly manipulate hardware, execute privileged commands, or own system policy.
2. Android is a first-class application workload, not a late optional add-on.
3. Linux applications remain supported through a separate native application provider.
4. Android runtime implementation is isolated behind an adapter. Aether must not become permanently coupled to one runtime implementation.
5. The graphical shell remains unprivileged. Privileged operations belong to narrowly scoped system services or established Linux services.
6. No feature is described as working against real hardware until its provider exists and has been tested.

## Logical components

### 1. Aether Shell

The existing Qt Quick application:
- draws the desktop, panels, launcher, dock, windows, and settings UI;
- displays state supplied by service adapters;
- sends user actions through stable interfaces;
- remains an unprivileged Wayland client.

Aether Shell is not currently a Wayland compositor. The initial platform plan uses a separate Wayland compositor/session (Weston is the current integration candidate) to host the shell and Android application surfaces. A future compositor decision is separate architecture work.

### 2. Application Compatibility Layer

The Application Compatibility Layer presents one catalogue and launch experience while routing each application to the appropriate provider.

**Providers**
- **Android provider:** controls an Android userspace runtime, initially evaluating Waydroid/container integration.
- **Linux provider:** discovers and launches native Linux desktop applications using XDG desktop-entry metadata.

**Shared responsibilities**
- unified application catalogue and stable application identifiers;
- application metadata, icons, categories, and search;
- launch requests and lifecycle status;
- install, update, and remove workflows appropriate to each application type;
- error reporting and capability reporting.

The Android provider must eventually cover runtime start/stop/status, APK discovery and installation, package removal and update, per-app launch, Android app metadata/icons, notifications, clipboard, file sharing, audio, input, display sizing, and permission handling. The Linux provider continues to use native Linux application mechanisms.

The current `ApplicationService` is an early Linux/XDG implementation. It is not yet the unified compatibility layer and does not yet manage APKs.

### 3. Android Runtime Service

This service owns the lifecycle and control interface for the selected Android runtime. The rest of Aether communicates through an adapter contract rather than invoking runtime-specific commands from QML.

The adapter contract should expose:
- runtime availability and detected capabilities;
- start, stop, restart, and status;
- Android package listing and package metadata;
- APK install/remove/update operations;
- launch and stop by Android package/activity identifier;
- runtime logs and structured failure information;
- integration state for notifications, clipboard, files, audio, input, and graphics.

Runtime-specific command execution must be implemented in a controlled backend. Do not expose arbitrary shell execution to QML.

### 4. Session services

User-session responsibilities that do not inherently require elevated privileges:
- **Application Catalog:** unified Android/Linux app discovery and launch routing;
- **Notification Service:** receives, stores, and presents notifications from Aether and supported providers;
- **Session Settings:** persists user preferences such as theme, accent colour, shell layout, startup behaviour, and user-level application preferences;
- **Session Manager:** coordinates shell session state and application/runtime lifecycle requests.

Session Settings is a distinct planned service and remains in the implementation sequence. It must persist preferences across shell restarts, validate values, provide defaults and migration/version handling, and distinguish user preferences from live hardware state. Theme and accent settings are the first persistence scope. Display brightness, Wi-Fi, Bluetooth, and power state are not ordinary theme preferences and must ultimately be controlled by system providers.

Session Manager must evolve beyond tracking one process per launched Linux app. It should coordinate provider-specific lifecycle handles: Linux process instances and Android package/activity instances managed by the Android runtime.

### 5. System services

System-wide information or privileged operations:
- **Network Service:** network devices and connection state, using NetworkManager or another selected backend;
- **Power Service:** battery, power-source, and supported power profiles, using Linux system interfaces such as UPower;
- **Display Service:** connected outputs, modes, and scaling through the active compositor/display protocol;
- **Device Service:** controlled inventory of relevant devices and capabilities;
- **Update Service:** trusted system updates through Debian package infrastructure initially.

These are logical responsibilities, not a requirement for one daemon per item. Related responsibilities may share a process where that is simpler and safer.

## Android runtime candidate and host requirements

Waydroid is the leading initial candidate because it runs a complete Android userspace in a Linux container and is designed to integrate Android applications into a Linux desktop. It uses Linux namespaces and a LineageOS-based Android system image. It is a candidate, not yet an adopted dependency.

Before adoption, verify:
- supported Debian release and packaging route;
- Binder/BinderFS availability and kernel configuration;
- namespaces, cgroups, seccomp, LXC/container requirements, and networking;
- GPU/DRM and hardware-accelerated graphics compatibility;
- Wayland compositor and per-application window integration;
- audio, input, clipboard, file-sharing, and notification behaviour;
- Android image update and maintenance policy;
- security model, SELinux policy, and permission boundaries.

The Android runtime shares the host Linux kernel; it is not a separate Android kernel. A suitable kernel configuration and device access are therefore essential. A container reduces the need for a full virtual machine, but does not eliminate the runtime's memory, storage, graphics, or kernel requirements.

### CPU and APK compatibility

Initial desktop target: x86-64. ARM64 is a planned platform target, not a currently verified build.

Android packages may contain native libraries for different ABIs (for example arm64-v8a, armeabi-v7a, x86_64, or x86). A runtime on x86-64 cannot automatically execute ARM-only native libraries. A translation layer may be investigated, but its licensing, availability, maintenance, security, and app-by-app compatibility must be established before it is included. Do not promise that every APK will run.

### Google services

AOSP/LineageOS-based Android userspace does not automatically imply Google Play certification or Google Mobile Services. The product must distinguish a Google-free Android image from any separately licensed or configured Google-services option. No Play Store availability is promised by this architecture.

### Security

Treat APKs and their data as untrusted. The eventual design must define:
- container isolation and least-privilege device access;
- SELinux and Android permission policy;
- separation of Android app data from Linux home data;
- controlled file-sharing and clipboard boundaries;
- network access and update trust;
- package provenance and signature validation;
- safe runtime shutdown and recovery.

## Wayland and graphics boundary

The current shell is a Qt Wayland client, not a compositor. Android app windows require a compositor and a runtime integration path that can present Android surfaces. Weston is a practical initial host candidate while the shell and runtime adapters are developed. Do not assume the existing QML shell alone can host or composite Android windows.

## Communication and privilege boundaries

- Prefer D-Bus for communication with established Linux system services and for Aether service APIs where appropriate.
- Keep runtime-specific commands inside the Android provider.
- Keep privileged operations in narrowly scoped system services or existing system daemons.
- Do not run the graphical shell as root.
- Validate every request at the service boundary; the UI is not a security boundary.
- Do not expose arbitrary shell-command execution as a general-purpose service API.
- Use typed state, asynchronous operation results, and useful error information.

## Initial interface contract

| Domain | Readable state | Requested operations |
|---|---|---|
| Applications | unified Android/Linux entries, provider, metadata, launchability | launch by stable application ID |
| Android runtime | availability, status, capabilities, installed packages | start/stop runtime; install/remove/update APK; launch/stop package |
| Notifications | active notifications, unread count, source | publish, dismiss, mark read |
| Session settings | persisted preferences and schema version | update validated user preference |
| Network | devices, connection state, available networks | connect, disconnect, radio control where supported |
| Power | battery, charging state, power source | request supported power profile |
| Display | connected outputs, modes, scale | request supported mode or scale |
| Devices | identity, type, capabilities | explicitly supported safe actions |

A UI control must not optimistically claim that an operation succeeded before the responsible service confirms it.

## Relationship to current source

- `System.qml` contains demonstration properties for Wi-Fi, Bluetooth, do-not-disturb, brightness, and battery. These are placeholders, not hardware integration.
- `ApplicationService.cpp/.h` discovers XDG desktop entries and delegates Linux process launch requests to `SessionManager`. It is not yet the unified Android/Linux catalogue.
- `AppCatalog.cpp/.h` provides the themed-icon image provider.
- `NotificationService.cpp/.h` provides an in-process notification model; external D-Bus notification integration remains future work.
- `SessionManager.cpp/.h` tracks launched Linux processes; it is not yet a complete login/session supervisor or Android lifecycle manager.
- `SessionSettings.cpp/.h` now persists dark/light mode and accent colour through QSettings. The existing Settings UI uses the service for appearance preferences. Other settings categories and migrations remain future work.
- `main.cpp` constructs the shell and current in-process services. These are not yet separate system daemons.

## Implementation sequence

1. **Record the Android-first compatibility architecture and runtime adapter boundary.**
2. **Extend Session Settings persistence** beyond the initial theme/accent implementation; add validated preference groups, migration tests, and user-level settings without redesigning the existing UI.
3. **Define the unified application model** so Android and Linux apps can coexist in the existing launcher.
4. **Implement the Android runtime adapter contract** and a candidate Waydroid backend behind it. Keep runtime-specific details out of QML.
5. **Extend Session Manager** to coordinate native process handles and Android runtime app handles.
6. **Integrate notification, clipboard, file, audio, input, and graphics bridges** one capability at a time.
7. **Integrate existing Linux system managers** for network, power, display, and devices.
8. **Add persistence migration, authorization, error handling, and security hardening.**
9. **Package for Debian** after source architecture is ready.
10. **Install, boot, and validate** only when the user decides to begin runtime testing.
11. **Port validated components into the preserved Yocto integration** later.

## Explicitly postponed

- Debian installation and boot configuration;
- QEMU or physical-device execution;
- runtime compilation and hardware validation;
- production installer and image generation;
- Yocto image builds.

Android runtime support is a primary development objective, but it is not implemented yet.

## Development constraints

- Preserve the current Aether Shell design and existing QML assets.
- Make changes on development branches; do not casually modify `main`.
- Keep Debian integration and Yocto image production separate from service implementation.
- Do not label a component implemented or tested until source changes exist and the relevant checks have actually been run.
