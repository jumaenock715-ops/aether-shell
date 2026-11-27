# Aether OS — Yocto integration (initial milestone)

This layer packages the existing Aether Shell Qt 6 / QML application. It does not
replace, rewrite, or mock the existing interface.

## Initial target

- Yocto Project 6.0 (Wrynose LTS)
- Machine: `qemux86-64`
- Init: systemd
- Display session: Weston / Wayland
- UI toolkit: Qt 6 with the meta-qt6 layer
- Image: `aether-image`

This is an initial integration scaffold. It is not yet a tested bootable image.
The QML runtime package list, Weston startup/session configuration, and machine
boot configuration must be validated during the first build.

## Layer dependencies

Use matching Wrynose branches for Poky and meta-openembedded. The meta-qt6
6.11 branch is the intended Qt layer for this integration. Check each layer's
compatibility metadata before building.

Expected layer set:
- `poky/meta`
- `poky/meta-poky`
- `poky/meta-yocto-bsp`
- `meta-openembedded/meta-oe`
- `meta-openembedded/meta-python`
- `meta-qt6`
- `meta-aether` (this directory)

## Build setup

On a suitably provisioned Linux build host:

```sh
git clone -b wrynose https://git.yoctoproject.org/poky
git clone -b wrynose https://git.openembedded.org/meta-openembedded
git clone -b 6.11 https://code.qt.io/yocto/meta-qt6

source poky/oe-init-build-env build-aether
```

Add the layers (adjust paths if your checkout layout differs):

```sh
bitbake-layers add-layer ../meta-openembedded/meta-oe
bitbake-layers add-layer ../meta-openembedded/meta-python
bitbake-layers add-layer ../meta-qt6
bitbake-layers add-layer ../meta-aether
```

Set the following in `conf/local.conf`:

```conf
MACHINE = "qemux86-64"
DISTRO = "aether"
PACKAGE_CLASSES = "package_ipk"
```

Then build:

```sh
bitbake aether-image
```

The expected image deploy directory is:

```
tmp/deploy/images/qemux86-64/
```

Use the generated image with the Yocto `runqemu` helper after checking the
actual image type and machine boot configuration:

```sh
runqemu qemux86-64 aether-image
```

## Important implementation notes

1. This milestone boots a standard Linux/Weston environment and packages the
   current Aether Shell. It does not implement a new Wayland compositor.
2. Automatic launch of Aether Shell from the Weston session is deliberately
   deferred until the first Weston boot is verified. This avoids assuming a
   particular seat, logind, or Weston service arrangement prematurely.
3. The shell's Wi-Fi, Bluetooth, brightness, battery, and notification state
   are still demonstration state in the current source. Real hardware control
   requires separate Aether system services and a defined IPC/API boundary.
4. The launcher currently discovers installed XDG desktop entries. The first
   image must include suitable base utilities and desktop files for a useful
   launcher experience.
5. Android application compatibility is not part of this image yet. It will
   require a separately validated Android runtime, kernel features, graphics
   integration, and ABI strategy.
6. The shell recipe pins the source revision used for this initial integration.
   Update that pin deliberately when promoting a tested shell revision.
7. The recipes use `LICENSE = "CLOSED"` for private development only. Before
   redistribution, choose the project's licence and provide the corresponding
   licence files and checksums.

## Build-host requirements

A full Yocto build is resource-intensive. The developer's current Windows
machine (4 GB RAM and 128 GB total storage) is not a suitable local build host
for this configuration. Use a capable Linux workstation or remote builder with
substantial free disk and memory.

## Status

- [x] Initial layer and image recipes committed to the development branch
- [ ] Validate layer compatibility and dependency resolution
- [ ] Complete first successful BitBake build
- [ ] Boot the image in QEMU
- [ ] Add and test Weston session autostart for Aether Shell
- [ ] Validate runtime QML module dependencies
- [ ] Begin ARM64 target work
