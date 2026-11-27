# Aether OS Android Runtime Integration

**Status:** Initial Waydroid CLI adapter implemented in source; no runtime has been installed or tested.  
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

The diagram describes intended boundaries, not implemented components.

### Unified application record

Each catalogue entry should have a stable Aether identifier and at least:
- application ID;
- provider type (`linux` or `android`);
- display name and description;
- icon reference;
- category;
- launch target (desktop ID for Linux; package/activity identity for Android);
- installation and launch state;
- provider capabilities.

Provider-specific metadata should remain available without making QML depend on provider-specific structures.

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

Initial integration should use a separate Wayland compositor. Weston is a candidate for the first development host. The Android runtime must be able to present its app surfaces through that compositor. Aether's launcher should start Android apps through the adapter, while the compositor/runtime integration handles their windows.

A future Aether compositor decision can be made independently. Do not turn the current Qt shell into a compositor by assumption.

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
2. Session Settings persistence — initial dark-mode and accent-colour persistence is implemented; extend the service and validate migrations.
3. Unified application record and dispatcher — first catalogue and provider-dispatch structure is implemented without changing the launcher visuals.
4. Runtime adapter skeleton — the Android provider interface is committed and remains independent of whether Waydroid is installed.
5. Waydroid CLI backend — initial asynchronous app-list, launch, APK install/remove, and user-session start/stop adapter is implemented. Runtime installation, image initialization, kernel configuration, and validation remain outstanding.
6. Notification, clipboard, file, audio, input, and window bridges.
7. Debian packaging and kernel configuration.
8. Runtime testing on real target hardware.
9. ARM64 platform and ARM ABI strategy.

## 11. Current implementation status

Not implemented:
- Android runtime or Android system image;
- ART integration;
- Binder/BinderFS configuration;
- Waydroid installation and Android system image;
- live Android package discovery and app operations have source implementations but remain untested; no confirmed working runtime is available;
- Android app windows and lifecycle integration;
- Android notification, clipboard, file, audio, or input bridges;
- ARM native-library translation.

Existing code:
- the Qt/QML shell and launcher;
- unified application catalogue/dispatch structure with Linux XDG discovery and a concrete, optional Waydroid adapter;
- preliminary Linux process launch;
- in-process notification model;
- preliminary Linux process tracking.

The Waydroid adapter invokes the documented `waydroid app list/install/remove/launch` and `waydroid session start/stop` commands as separate QProcess arguments; it never constructs a shell command. Its application-list parser follows the current Waydroid CLI source format (`Name`, `packageName`, `categories`). The adapter's `available` state means only that the executable was found; `running`/`stopped` reflect successful requests made through this adapter, not an independently polled health check. Listing depends on a running Waydroid session. No runtime build, boot, or hardware test has been performed.
