package launcher;

import org.libsdl.app.R;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.text.format.Formatter;
import android.view.View;
import android.widget.Button;
import android.widget.ListView;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

public class BiosActivity extends Activity {

    private static final int FILE_PICKER_REQUEST_CODE = 1338; 

    private ListView listView;
    private BiosAdapter adapter;
    private File biosDir;
    private List<BiosItem> biosList = new ArrayList<>();

    public native String[] getSupportedSystems();
    public native String getDefaultBiosPath(String configIniPath, String systemName);
    public native void setSystemBios(String configIniPath, String biosPath, String systemName);

    static {
        System.loadLibrary("main"); 
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_bios); 

        biosDir = new File(getFilesDir(), "BIOS");
        if (!biosDir.exists()) {
            biosDir.mkdirs();
        }

        listView = findViewById(R.id.bios_list_view);
        adapter = new BiosAdapter(this, biosList);
        listView.setAdapter(adapter);
        
        Button btnBack = findViewById(R.id.btnBackToLauncher);
        if (btnBack != null) {
            btnBack.setOnClickListener(v -> finish());
        }

        Button btnAddBios = findViewById(R.id.btnAddBios);
        if (btnAddBios != null) {
            btnAddBios.setOnClickListener(v -> {
                Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
                intent.setType("*/*");
                startActivityForResult(intent, FILE_PICKER_REQUEST_CODE);
            });
        }

        loadBiosFiles();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == FILE_PICKER_REQUEST_CODE && resultCode == RESULT_OK && data != null) {
            Uri uri = data.getData();
            if (uri != null) {
                copyBiosToInternalStorage(uri);
            }
        }
    }

    private void loadBiosFiles() {
        biosList.clear();
        File[] files = biosDir.listFiles();
        
        File configFile = new File(getFilesDir(), "config.ini");
        String configIniPath = configFile.getAbsolutePath();

        Map<String, String> defaultBiosMap = new HashMap<>();
        
        try {
            String[] systems = getSupportedSystems();
            if (systems != null) {
                for (String sys : systems) {
                    String defaultPath = getDefaultBiosPath(configIniPath, sys);
                    if (defaultPath != null && !defaultPath.equalsIgnoreCase("None") && !defaultPath.trim().isEmpty()) {
                        defaultBiosMap.put(defaultPath, sys);
                    }
                }
            }
        } catch (UnsatisfiedLinkError e) {
            e.printStackTrace();
        }

        if (files != null) {
            for (File file : files) {
                if (file.isFile()) {
                    String assignedSystem = defaultBiosMap.get(file.getAbsolutePath());
                    biosList.add(new BiosItem(file, file.lastModified(), assignedSystem));
                }
            }
        }

        Collections.sort(biosList, (a, b) -> Long.compare(b.lastModifiedTime, a.lastModifiedTime));
        adapter.notifyDataSetChanged();
    }

    private void copyBiosToInternalStorage(Uri uri) {
        try {
            String filename = null;
            
            if (uri.getScheme() != null && uri.getScheme().equals("content")) {
                try (Cursor cursor = getContentResolver().query(uri, null, null, null, null)) {
                    if (cursor != null && cursor.moveToFirst()) {
                        int index = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                        if (index != -1) {
                            filename = cursor.getString(index);
                        }
                    }
                }
            }
            
            if (filename == null) {
                String path = uri.getPath();
                if (path != null && path.contains("/")) {
                    filename = path.substring(path.lastIndexOf("/") + 1);
                } else {
                    filename = "imported_bios_" + System.currentTimeMillis() + ".bin";
                }
            }

            File destFile = new File(biosDir, filename);
            InputStream is = getContentResolver().openInputStream(uri);
            OutputStream os = new FileOutputStream(destFile);
            
            byte[] buffer = new byte[1024];
            int length;
            while ((length = is.read(buffer)) > 0) {
                os.write(buffer, 0, length);
            }
            
            os.flush();
            os.close();
            is.close();

            Toast.makeText(this, "Added: " + filename, Toast.LENGTH_SHORT).show();
            loadBiosFiles(); 

        } catch (Exception e) {
            e.printStackTrace();
            Toast.makeText(this, "Failed to copy BIOS", Toast.LENGTH_SHORT).show();
        }
    }

    public void showSystemSelectionMenu(BiosItem item) {
        try {
            final String[] systems = getSupportedSystems();
            
            if (systems == null || systems.length == 0) {
                Toast.makeText(this, "No systems available.", Toast.LENGTH_SHORT).show();
                return;
            }

            new AlertDialog.Builder(this)
                    .setTitle("Select System for BIOS")
                    .setItems(systems, (dialog, which) -> {
                        String selectedSystem = systems[which];
                        File configFile = new File(getFilesDir(), "config.ini");
                        
                        try {
                            setSystemBios(configFile.getAbsolutePath(), item.biosFile.getAbsolutePath(), selectedSystem);
                            Toast.makeText(this, "Set as " + selectedSystem + " BIOS", Toast.LENGTH_SHORT).show();
                            loadBiosFiles();
                        } catch (UnsatisfiedLinkError e) {
                            e.printStackTrace();
                            Toast.makeText(this, "Native library error on setting system", Toast.LENGTH_SHORT).show();
                        }
                    })
                    .setNegativeButton("Cancel", null)
                    .show();
                    
        } catch (UnsatisfiedLinkError e) {
            e.printStackTrace();
            Toast.makeText(this, "Native library error on getting systems", Toast.LENGTH_SHORT).show();
        }
    }

    public void showBiosInfo(BiosItem item) {
        File f = item.biosFile;
        String size = Formatter.formatFileSize(this, f.length());
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault());
        String modified = sdf.format(new Date(item.lastModifiedTime));

        String info = "Name: " + f.getName() + "\n"
                    + "Size: " + size + "\n"
                    + "Modified: " + modified + "\n"
                    + "Path: " + f.getAbsolutePath();

        new AlertDialog.Builder(this)
                .setTitle("BIOS Info")
                .setMessage(info)
                .setPositiveButton("Close", null)
                .show();
    }

    public void deleteBiosFile(BiosItem item) {
        new AlertDialog.Builder(this)
                .setTitle("Delete BIOS")
                .setMessage("Are you sure you want to delete '" + item.biosFile.getName() + "'?")
                .setPositiveButton("Delete", (dialog, which) -> {
                    if (item.biosFile.delete()) {
                        Toast.makeText(this, "Deleted: " + item.biosFile.getName(), Toast.LENGTH_SHORT).show();
                        loadBiosFiles(); 
                    } else {
                        Toast.makeText(this, "Failed to delete BIOS.", Toast.LENGTH_SHORT).show();
                    }
                })
                .setNegativeButton("Cancel", null)
                .show();
    }
}
