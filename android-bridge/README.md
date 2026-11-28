# Aether Android Bridge

This optional companion is installed **inside the Waydroid Android environment**. It is not the Android runtime itself.

## Features

- Forwards posted and dismissed Android notifications to the host through the app-specific shared-storage directory.
- Synchronises plain-text clipboard content while the companion activity is open. Android restricts background clipboard access, so this is intentionally foreground-only.
- Does not request broad storage permissions. It writes only to its own app-specific external-files directory.

## Build

Build the APK from this directory with Android SDK 35 and Gradle 8.7 or newer:

```sh
gradle :app:assembleDebug
```

Install the generated APK in Waydroid, launch **Aether Bridge**, grant notification access, and open the app's clipboard-sync control when clipboard sharing is needed.

The host shell looks for the app's files at:

`<Waydroid media root>/Android/data/org.aetheros.bridge/files/`

The Waydroid media root is normally `~/.local/share/waydroid/data/media/0` or `/var/lib/waydroid/data/media/0`. The exact host path and permissions depend on the Waydroid installation.

## Security and limitations

Notification text is copied to the host notification centre. Notification actions, inline replies, images and Android notification channels are not forwarded yet. Clipboard sync is text-only and stops when the companion activity is not in the foreground. This is a companion bridge, not a privileged Android system service.
