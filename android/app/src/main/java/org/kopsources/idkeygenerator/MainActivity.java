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
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.Base64;

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
        generate.setText("Generate keys");
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
            byte[] ike = Base64.getEncoder().encode(plain);
            byte[] ike2 = Base64.getEncoder().encode(ike);
            saveDownload("id.ike", ike);
            saveDownload("id.ike2", ike2);
            status.setText("Saved id.ike and id.ike2 to Downloads.");
        } catch (Exception e) {
            status.setText("Could not generate key files.");
        }
    }

    private void saveDownload(String name, byte[] contents) throws Exception {
        ContentValues values = new ContentValues();
        values.put(MediaStore.Downloads.DISPLAY_NAME, name);
        values.put(MediaStore.Downloads.MIME_TYPE, "application/octet-stream");
        values.put(MediaStore.Downloads.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS);
        try (OutputStream out = getContentResolver().openOutputStream(
                getContentResolver().insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, values))) {
            if (out == null) throw new java.io.IOException("No output stream");
            out.write(contents);
        }
    }

    private int dp(int value) {
        return (int) (value * getResources().getDisplayMetrics().density + 0.5f);
    }
}
