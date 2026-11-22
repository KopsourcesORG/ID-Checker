// SPDX-License-Identifier: GPL-3.0-only
package org.kopsources.idkeygenerator;

import android.app.Activity;
import android.content.ContentValues;
import android.os.Bundle;
import android.os.Environment;
import android.provider.MediaStore;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.util.TypedValue;

import java.io.OutputStream;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.Base64;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

public final class MainActivity extends Activity {
    private EditText boardId;
    private TextView status;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        int pad = dp(20);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(pad, pad, pad, pad);
        root.setGravity(android.view.Gravity.CENTER_VERTICAL);

        TextView title = new TextView(this);
        title.setText("ID Key Generator");
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 21);
        root.addView(title, new LinearLayout.LayoutParams(-1, -2));

        boardId = new EditText(this);
        boardId.setSingleLine(true);
        boardId.setHint("Mainboard ID");
        boardId.setInputType(android.text.InputType.TYPE_CLASS_TEXT | android.text.InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        LinearLayout.LayoutParams inputParams = new LinearLayout.LayoutParams(-1, -2);
        inputParams.topMargin = dp(16);
        root.addView(boardId, inputParams);

        Button generate = new Button(this);
        generate.setText("Generate flashable ZIP");
        LinearLayout.LayoutParams buttonParams = new LinearLayout.LayoutParams(-1, -2);
        buttonParams.topMargin = dp(8);
        root.addView(generate, buttonParams);

        status = new TextView(this);
        status.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        LinearLayout.LayoutParams statusParams = new LinearLayout.LayoutParams(-1, -2);
        statusParams.topMargin = dp(8);
        root.addView(status, statusParams);

        generate.setOnClickListener(view -> generateKeys());
        setContentView(root, new ViewGroup.LayoutParams(-1, -1));
    }

    private void generateKeys() {
        String id = boardId.getText().toString().trim();
        if (id.isEmpty()) {
            status.setText("Enter a mainboard ID.");
            return;
        }
        try {
            byte[] digest = MessageDigest.getInstance("SHA-256").digest(id.getBytes(StandardCharsets.UTF_8));
            StringBuilder hex = new StringBuilder(64);
            for (byte value : digest) hex.append(String.format(java.util.Locale.ROOT, "%02x", value & 0xff));
            byte[] plain = ("IDC1\n" + hex + "\n").getBytes(StandardCharsets.US_ASCII);
            byte[] ike2 = Base64.getEncoder().encode(Base64.getEncoder().encode(plain));
            saveFlashableZip(ike2);
            status.setText("Saved flashable ZIP to Downloads.");
        } catch (Exception e) {
            status.setText("Could not create flashable ZIP.");
        }
    }

    private void saveFlashableZip(byte[] ike2) throws Exception {
        String name = "ID-Key-" + System.currentTimeMillis() + ".zip";
        ContentValues values = new ContentValues();
        values.put(MediaStore.Downloads.DISPLAY_NAME, name);
        values.put(MediaStore.Downloads.MIME_TYPE, "application/zip");
        values.put(MediaStore.Downloads.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS);
        android.net.Uri uri = getContentResolver().insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, values);
        if (uri == null) throw new java.io.IOException("Could not create download");
        try {
            try (OutputStream raw = getContentResolver().openOutputStream(uri)) {
                if (raw == null) throw new java.io.IOException("No output stream");
                try (ZipOutputStream out = new ZipOutputStream(raw)) {
                    addAsset(out, "META-INF/com/google/android/update-binary");
                    addAsset(out, "META-INF/com/google/android/updater-script");
                    addAsset(out, "id-key-flasher");
                    out.putNextEntry(new ZipEntry("id.ike2"));
                    out.write(ike2);
                    out.closeEntry();
                    out.finish();
                }
            }
        } catch (Exception e) {
            getContentResolver().delete(uri, null, null);
            throw e;
        }
    }

    private void addAsset(ZipOutputStream out, String path) throws Exception {
        out.putNextEntry(new ZipEntry(path));
        try (InputStream in = getAssets().open(path)) {
            byte[] buffer = new byte[8192];
            int count;
            while ((count = in.read(buffer)) != -1) out.write(buffer, 0, count);
        }
        out.closeEntry();
    }

    private int dp(int value) {
        return (int) (value * getResources().getDisplayMetrics().density + 0.5f);
    }
}
