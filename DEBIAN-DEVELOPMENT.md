# Aether OS — Debian Development Path

This document defines the immediate development path for Aether OS. Debian is the development and early integration base. The existing Yocto integration remains preserved on the separate `yocto/initial-image` branch for later image production.

## Scope

- Preserve the existing Qt 6 / QML Aether Shell and its graphics.
- Build and run the shell on Debian before adding OS-level integrations.
- Use Debian's packaged kernel, drivers, systemd and Wayland components during early development.
- Develop Aether-specific system services incrementally.
- Do not treat the current shell demo controls as real hardware controls; `System.qml` is still demonstration state.
- Android application compatibility is a later project phase, not part of this initial setup.

## Recommended development base

Use Debian 13 (Trixie) amd64. The project requires Qt 6.5 or newer because it uses `QQmlApplicationEngine::loadFromModule`; Debian 13's Qt 6 packages satisfy that requirement.

On a 4 GB RAM computer, prefer a native Debian installation or a carefully limited environment over a large virtual machine. Back up important data before changing partitions or installing another operating system. Do not repartition the Windows drive until the user has deliberately chosen that route.

## Install build and runtime dependencies

Open a Debian terminal:

```sh
sudo apt update
sudo apt install --no-install-recommends \
  git build-essential cmake ninja-build pkg-config \
  qt6-base-dev qt6-declarative-dev \
  qml6-module-qtquick qml6-module-qtquick-window \
  weston
```

If the Debian installation is headless or does not already have a graphical session, Weston can provide a Wayland compositor for testing. On a normal Debian desktop session, first try running the shell in the existing session.

## Get the source

```sh
git clone https://github.com/jumaenock715-ops/aether-shell.git
cd aether-shell
git switch main
```

Keep local development changes on a separate branch rather than committing experimental changes directly to `main`:

```sh
git switch -c debian/first-run
```

## Configure and compile conservatively

On a 4 GB RAM machine, compile with one job to reduce memory pressure:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 1
```

If CMake reports that Qt 6.5 or newer cannot be found, check the installed version:

```sh
qmake6 -query QT_VERSION
```

Do not lower the Qt requirement or rewrite the application to work around an old package without first reviewing the actual compatibility issue.

## Run Aether Shell

From a graphical Wayland session:

```sh
QT_QPA_PLATFORM=wayland ./build/aether-shell
```

If running under X11 instead, try:

```sh
QT_QPA_PLATFORM=xcb ./build/aether-shell
```

For a separate Weston test session, log out of the desktop session first if necessary, then start Weston from a suitable local console:

```sh
weston
```

Open a terminal inside Weston and run the shell from its build directory. Do not start a nested Weston session blindly inside an existing Wayland session.

## Initial verification checklist

- [ ] CMake detects Qt 6.5 or newer.
- [ ] The project compiles without modifying the existing QML interface.
- [ ] Aether Shell launches and displays its existing desktop.
- [ ] Launcher, dock, floating windows, quick settings and notifications are exercised.
- [ ] Any runtime QML import errors are recorded and resolved by adding the correct Debian runtime package.
- [ ] Hardware-facing controls are clearly treated as demo-only until connected to actual Linux services.

## Development sequence after first launch

1. Record the exact Debian version, kernel, Qt version and graphics backend.
2. Establish a clean build and repeatable launch procedure.
3. Add a small Aether system-service layer with well-defined interfaces.
4. Connect one real system capability at a time (for example, notifications or application launching), retaining safe fallbacks.
5. Add session startup and login integration only after the shell is stable.
6. Later, evaluate a Debian-based installer/image and revisit Yocto for a more tightly controlled product image.

## Yocto preservation

The Yocto scaffold is intentionally not removed or merged into this branch. It remains on:

`yocto/initial-image`

That branch is an unvalidated integration scaffold, not a confirmed bootable image. Debian development does not invalidate or replace that work.
