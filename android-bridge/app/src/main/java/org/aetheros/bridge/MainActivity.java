package org.aetheros.bridge;

import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.text.TextUtils;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;

public final class MainActivity extends Activity {
    private final Handler handler = new Handler(Looper.getMainLooper());
    private ClipboardManager clipboard;
    private String lastAndroidText = "";
    private String lastHostText = "";
    private boolean syncing = false;

    private final Runnable pollHostClipboard = new Runnable() {
        @Override public void run() {
            if (!syncing) return;
            readHostClipboard();
            handler.postDelayed(this, 700);
        }
    };

    private File bridgeFile(String name) {
        File root = getExternalFilesDir(null);
        if (root == null) return null;
        if (!root.exists() && !root.mkdirs()) return null;
        return new File(root, name);
    }

    private void writeText(String name, String value) {
        File file = bridgeFile(name);
        if (file == null) return;
        File temp = new File(file.getParentFile(), file.getName() + ".tmp");
        try (FileOutputStream out = new FileOutputStream(temp, false)) {
            out.write(value.getBytes(StandardCharsets.UTF_8));
            out.flush();
            out.getFD().sync();
        } catch (Exception ignored) { return; }
        if (file.exists() && !file.delete()) { temp.delete(); return; }
        if (!temp.renameTo(file)) temp.delete();
    }

    private String readText(String name) {
        File file = bridgeFile(name);
        if (file == null || !file.isFile()) return null;
        try (FileInputStream in = new FileInputStream(file)) {
            byte[] data = new byte[(int)Math.min(file.length(), 1024 * 1024)];
            int length = in.read(data);
            return length <= 0 ? "" : new String(data, 0, length, StandardCharsets.UTF_8);
        } catch (Exception ignored) {
            return null;
        }
    }

    private void readHostClipboard() {
        String value = readText("clipboard-to-android.txt");
        if (value == null || value.isEmpty() || value.equals(lastHostText) || value.equals(lastAndroidText))
            return;
        if (clipboard == null) clipboard = (ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
        if (clipboard == null) return;
        lastHostText = value;
        lastAndroidText = value;
        clipboard.setPrimaryClip(ClipData.newPlainText("Aether", value));
    }

    private final ClipboardManager.OnPrimaryClipChangedListener clipboardListener = () -> {
        if (!syncing || clipboard == null || !clipboard.hasPrimaryClip()) return;
        ClipData clip = clipboard.getPrimaryClip();
        if (clip == null || clip.getItemCount() == 0) return;
        CharSequence text = clip.getItemAt(0).coerceToText(this);
        if (text == null) return;
        String value = text.toString();
        if (TextUtils.isEmpty(value) || value.equals(lastAndroidText)) return;
        lastAndroidText = value;
        writeText("clipboard-from-android.txt", value);
    };

    @Override protected void onCreate(Bundle state) {
        super.onCreate(state);
        clipboard = (ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
        File bridgeRoot = getExternalFilesDir(null);
        if (bridgeRoot != null) bridgeRoot.mkdirs();
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(32, 48, 32, 32);
        TextView title = new TextView(this);
        title.setText("Aether Bridge");
        title.setTextSize(24);
        layout.addView(title);
        TextView info = new TextView(this);
        info.setText("Forward Android notifications to Aether OS and synchronise text clipboard while this app is open.");
        layout.addView(info);
        Button grant = new Button(this);
        grant.setText("Grant notification access");
        grant.setOnClickListener(v -> startActivity(new Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS)));
        layout.addView(grant);
        Button clipboardButton = new Button(this);
        clipboardButton.setText("Enable foreground clipboard sync");
        clipboardButton.setOnClickListener(v -> {
            syncing = true;
            info.setText("Clipboard sync active while this screen remains open. Notification forwarding runs independently after access is granted.");
            startClipboardSync();
        });
        layout.addView(clipboardButton);
        setContentView(layout);
    }

    private void startClipboardSync() {
        if (clipboard != null) {
            try { clipboard.removePrimaryClipChangedListener(clipboardListener); } catch (Exception ignored) { }
            clipboard.addPrimaryClipChangedListener(clipboardListener);
        }
        handler.removeCallbacks(pollHostClipboard);
        handler.post(pollHostClipboard);
    }

    @Override protected void onResume() {
        super.onResume();
        // Keep the file bridge active only while the user has the companion open.
        if (syncing) startClipboardSync();
    }

    @Override protected void onPause() {
        super.onPause();
        handler.removeCallbacks(pollHostClipboard);
        if (clipboard != null) {
            try { clipboard.removePrimaryClipChangedListener(clipboardListener); } catch (Exception ignored) { }
        }
    }
}
