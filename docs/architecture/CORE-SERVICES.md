# Aether OS Core Services — Architecture Foundation

**Status:** Initial session-service implementation in progress.  
**Target base:** Debian initially; Yocto integration remains preserved separately.  
**Current shell:** Existing Qt 6 / QML Aether Shell.

## Purpose

This document establishes the boundary between Aether Shell and the operating-system services it will eventually use. It is deliberately a design and source-development step: no Debian installation, image build, boot, or runtime test is required at this stage.

The existing shell is retained. Its visual components and interaction model are not to be replaced as part of the core-services work.

## Architectural rule

QML presents state and requests actions. It must not directly manipulate hardware, invoke privileged commands, or own system policy.

System operations belong behind service interfaces. Services obtain authoritative state from Linux facilities and report results back to the shell. Until a real provider is implemented, the existing demo values in `System.qml` remain explicitly demo-only; they must not be represented as actual device state.

## Logical components

### 1. Aether Shell

The existing Qt Quick application:
- draws the desktop, panels, launcher, dock, windows, and settings UI;
- displays state supplied by service adapters;
- sends user actions through narrow, documented interfaces;
- remains unprivileged.

### 2. Aether session services

User-session functions that do not require elevated privileges:
- **Application Catalog:** discovers installed applications and launches them using desktop-entry metadata;
- **Notification Service:** receives, stores, and presents notifications;
- **Session Settings:** stores user preferences such as theme, accent, and shell layout;
- **Session Manager:** tracks session state and coordinates shell-level actions.

### 3. System services

Operations that need system-wide information or privileged access:
- **Network Service:** reports network devices and connection state; requests connection changes through NetworkManager or another selected Linux network backend;
- **Power Service:** reports battery, power-source, and power-profile information through system interfaces such as UPower where available;
- **Display Service:** reports displays and supported modes; requests changes through the active compositor/display protocol;
- **Device Service:** exposes a controlled inventory of relevant devices and capabilities;
- **Update Service:** later coordinates trusted system updates through Debian package infrastructure.

These are logical responsibilities, not a requirement to create one daemon per item. Related responsibilities may share a process where that keeps the system simpler and safer.

## Communication and privilege boundaries

- Prefer D-Bus for communication with established Linux system services and for Aether service APIs where appropriate.
- Use existing Linux services rather than writing new hardware drivers or duplicating mature system managers.
- Keep privileged operations in narrowly scoped system services or existing system daemons.
- Do not run the graphical shell as root.
- Validate every request at the service boundary; the UI is not a security boundary.
- Do not expose arbitrary shell-command execution as a general-purpose service API.

## Initial interface contract

The shell-facing API should use typed properties, signals, and explicit asynchronous operation results.

| Domain | Readable state | Requested operations |
|---|---|---|
| Applications | installed entries, launchability | launch an application by stable ID |
| Notifications | active notifications, unread count | publish, dismiss, mark read |
| Network | devices, connection state, available networks | connect, disconnect, enable/disable radio where supported |
| Power | battery, charging state, power source | request supported power profile |
| Display | connected outputs, modes, scale | request a supported mode or scale |
| Settings | persisted user preferences | update a validated preference |
| Devices | device identity, type, capabilities | only explicitly supported safe actions |

Operations that can fail must return a result and useful error information. A UI toggle must not optimistically claim success before the service confirms the change.

## Relationship to current source

- `System.qml` currently contains demonstration properties for Wi-Fi, Bluetooth, do-not-disturb, brightness, and battery. These are placeholders, not hardware integration.
- `ApplicationService.cpp/.h` discovers XDG desktop entries and delegates launch requests to `SessionManager`; `AppCatalog.cpp/.h` now provides the themed-icon image provider. Desktop-entry handling still needs further specification work.
- `NotificationService.cpp/.h` now provides the in-process notification model; a later adapter can connect it to the desktop notification service.
- `SessionManager.cpp/.h` now tracks launched application processes; it is not yet a full login/session supervisor.
- `main.cpp` constructs the shell, application service, notification service, and session manager. These are currently in-process components, not separate system daemons.

## Implementation sequence

1. **Define service contracts and data types.** Keep responsibilities and privilege boundaries explicit.
2. **Extract shell-facing adapters.** Keep QML dependent on stable interfaces rather than Linux implementation details.
3. **Implement session-level services first.** Application discovery/launching, in-process notifications, and initial process lifecycle tracking are now present; continue with session settings and desktop notification integration.
4. **Integrate existing Linux system managers.** Add network, power, and display providers one domain at a time.
5. **Add persistence, authorization, and error handling.**
6. **Package for Debian.** This is a later integration stage, not a prerequisite for source development.
7. **Validate at runtime.** Only after the source architecture is sufficiently complete do we install, run, and test it.
8. **Port the validated components into the preserved Yocto integration.**

## Explicitly deferred

- Debian installation and boot configuration;
- QEMU or physical-device execution;
- custom kernel and hardware-driver development;
- Android runtime/container integration;
- production installer and image generation;
- Yocto image builds.

## Development constraints

- Preserve the current Aether Shell design and existing QML assets.
- Make changes on development branches; do not casually modify `main`.
- Keep Debian integration and Yocto image production separate from service implementation.
- Do not label a component implemented or tested until source changes exist and the relevant checks have actually been run.
