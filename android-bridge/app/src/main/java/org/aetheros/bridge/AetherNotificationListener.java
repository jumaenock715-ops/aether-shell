package org.aetheros.bridge;

import android.app.Notification;
import android.service.notification.NotificationListenerService;
import android.service.notification.StatusBarNotification;

import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;

public final class AetherNotificationListener extends NotificationListenerService {
    private final Object lock = new Object();

    private File eventFile() {
        File root = getExternalFilesDir(null);
        if (root == null) return null;
        if (!root.exists() && !root.mkdirs()) return null;
        return new File(root, "notifications.ndjson");
    }

    private void append(JSONObject event) {
        File file = eventFile();
        if (file == null) return;
        synchronized (lock) {
            try (FileOutputStream out = new FileOutputStream(file, true)) {
                out.write(event.toString().getBytes(StandardCharsets.UTF_8));
                out.write('\n');
                out.flush();
            } catch (Exception ignored) {
                // The host bridge polls this append-only file; failure is non-fatal
                // to Android notification delivery inside the container.
            }
        }
    }

    @Override
    public void onNotificationPosted(StatusBarNotification sbn) {
        if (sbn == null || getPackageName().equals(sbn.getPackageName())) return;
        Notification notification = sbn.getNotification();
        if (notification == null || notification.extras == null) return;
        CharSequence title = notification.extras.getCharSequence(Notification.EXTRA_TITLE);
        CharSequence text = notification.extras.getCharSequence(Notification.EXTRA_BIG_TEXT);
        if (text == null) text = notification.extras.getCharSequence(Notification.EXTRA_TEXT);
        try {
            JSONObject event = new JSONObject();
            event.put("event", "posted");
            event.put("key", sbn.getKey());
            event.put("packageName", sbn.getPackageName());
            event.put("title", title == null ? "" : title.toString());
            event.put("text", text == null ? "" : text.toString());
            event.put("postedAt", sbn.getPostTime());
            append(event);
        } catch (Exception ignored) { }
    }

    @Override
    public void onNotificationRemoved(StatusBarNotification sbn) {
        if (sbn == null) return;
        try {
            JSONObject event = new JSONObject();
            event.put("event", "removed");
            event.put("key", sbn.getKey());
            event.put("packageName", sbn.getPackageName());
            append(event);
        } catch (Exception ignored) { }
    }
}
