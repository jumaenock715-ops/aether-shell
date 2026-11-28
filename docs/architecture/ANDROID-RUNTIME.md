# Aether OS Android Runtime Integration

**Status:** Wayland foreign-toplevel tracker and explicit Android window-identity resolver implemented in source; no build or runtime validation has been performed.  
**Initial candidate:** Waydroid, subject to technical validation.  
**Initial host direction:** Debian + Wayland + existing Aether Shell.

## 1. Objective

Android applications are Aether OS's primary application workload. Aether should provide a coherent experience in which Android and native Linux applications appear in one launcher and can be started through one application-facing API.

Aether is not attempting to reimplement Android. The initial strategy is to host a maintained Android userspace/runtime and build Aether-owned integration around it.

## 2. Candidate assessment

### Waydroid — leading candidate

Waydroid runs a complete Android userspace in a Linux container, using Linux namespaces and a LineageOS-derived Android image. It is intended to run on a regular GNU/Linux system and provides application operations such as install, remove, launch, and list.

Why it fits:
- Android can coexist with the Debian/Linux host rather than replacing it.
- Container execution avoids running a full guest kernel and conventional VM for the normal desktop use case.
- It has a command-line control surface that can be wrapped by an Aether service.
- It is designed for Wayland desktop integration.

Risks and constraints:
- Requires suitable host kernel features and Binder support.
- Graphics integration depends on the compositor and GPU/driver stack.
- Android userspace and images have their own update lifecycle.
- ARM-only APKs do not automatically run on an x86-64 host.
- Google Play/GMS is not guaranteed by choosing a LineageOS-based image.
- The runtime's security and device-access policy must be reviewed before production use.

### Other approaches

- **Full Android virtual machine:** useful as a fallback or test target, but adds virtualization overhead and can make desktop window integration less direct.
- **Android-x86 / Android-derived PC distributions:** run Android as the main OS rather than providing the cleanest Android-plus-Linux application environment. They are not the initial direction for this Debian-based shell.
- **Anbox:** do not select as the foundation without a fresh maintenance and compatibility assessment; Waydroid is the stronger current candidate.

Decision: design a replaceable provider interface now; make the final runtime selection only after a Debian/kernel/Wayland compatibility review.

## 3. Proposed component model

```text
                         Aether Shell (Qt/QML)
                                  |
                       Unified App Catalogue
                                  |
                       Application Dispatcher
                         /               \
              Linux App Provider     Android Provider
                     |                    |
             XDG desktop entries    Android Runtime Adapter
                     |                    |
             Session Manager        Waydroid candidate
                     |                    |
             Native Linux apps      Android system + ART
                                          |
                                    APK applications
```

The diagram describes the intended provider boundaries. The initial Android session manager described below is now present in source.

### Android application session tracking

`AndroidSessionManager` is a provider-facing list model exposed to QML as `AndroidSessions`. It records one request record per package, including display name, last request time, request outcome, explanatory detail, and a separate `activityState` field.

Request states:
- `launch-requested` — Aether submitted the request to the runtime adapter;
- `launch-command-completed` — the Waydroid CLI command completed successfully; this does **not** prove that an Android activity is running or visible;
- `launch-failed` — the adapter reported a target-correlated failure;
- `runtime-stopped` — the adapter reported completion of a user-session stop request.

The model also aggregates compositor-visible windows per package. It accepts provider events `opened`, `activated`, `deactivated`, and `closed`, tracks multiple opaque window IDs per package, and exposes `windowCount`, `activeWindowCount`, and `windowState` (`foreground`, `background`, `mapped`, `closed`, or `unknown`). This is deliberately distinct from Android process/activity lifetime: a visible window is evidence of a mapped desktop surface, not proof of every Android service or background process state.

The adapter contract has two optional event channels:
- `applicationLifecycleChanged(packageName, state)` for authoritative runtime activity events (`started`, `foreground`, `background`, `stopped`, `crashed`);
- `applicationWindowChanged(packageName, windowId, title, state)` for compositor window events.

A provider must resolve Android package identity without guessing from window titles or arbitrary compositor app IDs. The Wayland tracker supplies raw window events to the Waydroid adapter. The adapter first checks an explicit live-window-ID-to-package mapping, which is accepted only for a currently observed window and an installed package. For standard Waydroid multi-window surfaces, it can also resolve the vendor-defined `waydroid.<package-name>` app-ID namespace. Waydroid's HWC constructs that namespace from its internal app identifier, and its window-control HAL compares that identifier with the supplied Android package name. Aether strips only the exact, case-sensitive `waydroid.` prefix and validates the suffix against the installed-app catalogue. It does not match titles or arbitrary app IDs. This automatic path depends on Waydroid multi-window mode being enabled (`persist.waydroid.multi_windows=true`) and remains subject to validation against the actual image and compositor. Explicit mappings remain available for other trusted runtime bridges; mappings are in-memory only. Activity state remains `unknown` because the Waydroid CLI adapter does not provide authoritative Android activity events.

### Wayland window observation strategy

Aether Shell is a Wayland client, not a compositor. `WaylandWindowTracker` now uses a dedicated Wayland client connection, runtime registry discovery, and Qt socket-notifier integration. It binds `ext-foreign-toplevel-list-v1` when advertised and reports `unsupported` when that global is unavailable or not exposed to the client. The current source does not yet implement the wlr foreign-toplevel fallback.

The standard `ext-foreign-toplevel-list-v1` protocol is intentionally minimal and compositor policy may restrict which clients can bind it. The tracker consumes mapped-window creation, metadata updates, and close events. It uses the protocol identifier when supplied and otherwise assigns a connection-local opaque identifier. This protocol does not report activation state, so the current tracker does not fabricate foreground/active events. It performs no window manipulation.

Current compatibility references list Weston 15 without either protocol, while Sway and labwc list support for `ext-foreign-toplevel-list-v1`. Consequently Weston is not an acceptable reference compositor for this feature. Labwc and Sway are candidates for the first protocol integration test, but Waydroid surface presentation and app identity still require validation. This does not commit Aether's final desktop session to either compositor.

A generic compositor `app_id` is not inherently an Android package name. The Waydroid-specific prefix above is accepted only because Waydroid's own HWC and window-control implementation establish that convention. `WaydroidRuntimeAdapter::registerWindowPackageMapping(windowId, packageName)` remains the explicit mapping entry point for other trusted bridges. It requires the exact window identifier to be currently observed, rejects invalid or uninstalled package identifiers, and removes the mapping when the window closes. The adapter re-associates a window when a mapping is registered or cleared and emits `unassociatedWindowChanged` when identity cannot be established. The adapter does not currently configure `persist.waydroid.multi_windows`; that property must be enabled in the Waydroid image and the session restarted before standard per-app windows can be observed.

These protocols report desktop toplevel windows, not Android's complete internal activity/process lifecycle. A mapped window can support truthful window-state reporting, but cannot prove that all activities or background services for that package are alive.

Waydroid's documented application CLI provides install, remove, launch, and list operations. It does not provide a documented per-application stop or live activity-state command in that interface. Therefore the current implementation does not invent per-app termination or parse unstable diagnostic output as authoritative lifecycle state.

### Android runtime adapter contract

The adapter should expose typed operations:
- `isAvailable()` and capability discovery;
- `start()`, `stop()`, `restart()`, `status()`;
- `listPackages()` and package metadata;
- `installApk(path)`, `removePackage(packageId)`, `updatePackage(path)`;
- `launch(packageId)`, `stopApp(packageId)`;
- runtime log retrieval and structured errors.

All operations must be asynchronous from the UI's perspective. The backend must validate paths, package identifiers, runtime state, and permissions. Never pass arbitrary QML strings to a shell.

## 4. Host and kernel prerequisites

The selected runtime must be checked against the actual Debian kernel and target hardware before packaging. At minimum, investigate:
- Binder IPC and BinderFS configuration/device nodes;
- Linux namespaces and cgroups;
- seccomp and LXC/container support;
- networking and required firewall/NAT behaviour;
- shared memory and Android IPC;
- DRM render nodes and GPU driver compatibility;
- audio, input, and device permissions;
- Wayland compositor integration.

The Android userspace shares the host Linux kernel. It does not bring an independent Android kernel with it. Kernel support and device access therefore belong in Aether's platform requirements, not in a late optional installer script.

## 5. Wayland and window integration

Aether Shell currently runs as a Wayland client. It is not a compositor.

Initial integration must use a separate Wayland compositor that exposes a supported foreign-toplevel protocol. Weston was previously considered, but the current protocol compatibility references do not list Weston 15 as supporting either `ext-foreign-toplevel-list-v1` or `zwlr-foreign-toplevel-management-unstable-v1`; it is therefore not a suitable reference compositor for this window-observation implementation. Labwc and Sway are candidate development compositors because current compatibility references list support for the standard foreign-toplevel list protocol. This is a protocol-capability choice, not yet a final desktop UX choice. Validate Waydroid surface presentation and package identity mapping on the selected compositor before adoption.

Aether's launcher starts Android apps through the adapter, while the compositor/runtime integration handles their windows. Aether Shell remains a Wayland client, not a compositor. A future Aether compositor decision can be made independently.

## 6. CPU ABI compatibility

Initial desktop target is x86-64. ARM64 is a later target.

An APK may include Java/Kotlin bytecode and native libraries compiled for one or more Android ABIs. Native libraries must match the runtime's supported ABI or be translated. Therefore:
- native x86-64 APKs are the straightforward initial target on x86-64;
- ARM-only APKs are a known compatibility gap;
- ARM translation layers require a separate legal, security, maintenance, and compatibility review;
- app compatibility must be measured by actual app tests, not inferred from successful APK installation.

Do not claim universal APK compatibility.

## 7. Android services and desktop bridges

Plan the following as separate integration capabilities:
1. app discovery and metadata;
2. APK install/update/remove;
3. app launch and lifecycle;
4. Android notification forwarding into Aether Notification Service;
5. clipboard bridge with explicit user-session boundaries;
6. file access through a controlled shared directory or document-provider bridge;
7. audio routing;
8. keyboard, pointer, touch, and controller input;
9. display scaling, resolution, rotation, and multi-window behaviour;
10. Android runtime settings and permissions.

Implement and validate these independently. A runtime being able to launch one app does not mean all these bridges work.

## 8. Security and trust

- Treat downloaded APKs and app data as untrusted.
- Preserve Android application sandboxing and permission controls.
- Limit container device access to what the runtime needs.
- Keep Android app-private data separate from the Linux home directory.
- Make file and clipboard sharing explicit and scoped.
- Verify package signatures and installation source where possible.
- Define update trust and rollback/recovery behaviour.
- Never run Aether Shell as root to make Android integration easier.

## 9. Google services

A LineageOS/AOSP-derived runtime does not automatically include licensed Google Mobile Services or a certified Play Store. Aether must not advertise Play Store support until the image, licensing, certification, and account requirements are resolved. A Google-free configuration should remain possible.

## 10. Development milestones

1. Architecture and provider interface — documented.
2. Session Settings persistence — initial dark-mode, accent-colour, and do-not-disturb preferences are implemented; extend the service and validate migrations.
3. Unified application record and dispatcher — first catalogue and provider-dispatch structure is implemented without changing the launcher visuals.
4. Runtime adapter skeleton — the Android provider interface is committed and remains independent of whether Waydroid is installed.
5. Waydroid CLI backend — asynchronous app-list, launch, APK install/remove, and user-session start/stop operations are implemented. Launcher requests start the user session automatically when needed; Settings also exposes manual session controls and refresh. Runtime installation, image initialization, kernel configuration, and validation remain outstanding.
6. Android request ledger, Wayland foreign-toplevel tracker, and explicit package identity resolver — source implementation present. The ext protocol backend and mapping registry are connected; automatic Waydroid metadata-to-package mapping and the wlr protocol fallback remain outstanding. Weston is excluded as the reference compositor for this feature; labwc/Sway are candidates.
7. Notification, clipboard, file, audio, and input bridges.
8. Debian packaging and kernel configuration.
9. Runtime testing on real target hardware.
10. ARM64 platform and ARM ABI strategy.

## 11. Current implementation status

Implemented in source:
- Qt/QML shell and launcher;
- unified application catalogue/dispatch structure with Linux XDG discovery and optional Waydroid adapter;
- preliminary Linux process launch tracking;
- in-process notification model;
- Session Settings persistence for dark mode, accent colour, and do-not-disturb;
- Android launch-request ledger;
- Android window-event contract and per-package multi-window aggregation.

Not implemented or not validated:
- Android runtime or Android system image;
- ART integration, Binder/BinderFS configuration, Waydroid installation and image initialization;
- real Wayland foreign-toplevel event source and package-identity resolver;
- authoritative Android activity lifecycle events;
- Android notification, clipboard, file, audio, or input bridges;
- per-Android-app termination;
- ARM native-library translation.

The Waydroid adapter invokes CLI operations as separate QProcess arguments, never as a constructed shell command. Its `available` state means only that the executable was found; `running`/`stopped` reflect successful requests made through this adapter, not an independently polled health check. The Wayland tracker is optional at build time and requires `wayland-client` plus `wayland-scanner`; without them it reports `unsupported-build`. No source build, runtime, boot, or hardware test has been performed.


## 7. Android and native desktop notifications

Aether's `NotificationService` now owns the freedesktop `org.freedesktop.Notifications` session-bus name and exports `/org/freedesktop/Notifications` when that name is available. Waydroid's host-side notification manager connects to this standard interface and forwards Android notifications through it; native Linux applications use the same endpoint. Aether stores incoming notifications in its existing list model, preserving the QML notification-centre interface. The initial server supports plain-text bodies, replacement IDs, close requests, and expiry. Rich body markup and interactive action UI are not advertised. If another notification daemon already owns the bus name, Aether records `name-in-use` and cannot receive desktop notifications until that conflict is resolved. This implementation has not been compiled or runtime-tested.
