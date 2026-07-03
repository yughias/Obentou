package launcher;

import emu.MainActivity;
import org.libsdl.app.R;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;
import androidx.core.content.FileProvider;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class LauncherActivity extends Activity {

    private static final int FILE_PICKER_REQUEST_CODE = 1337;
    
    private ListView listView;
    private RomAdapter adapter;
    private Spinner spinnerFilter;
    private EditText editTextSearch;
    private ImageView btnClearSearch;
    
    private File romDir;
    private List<RomItem> allRomList = new ArrayList<>();
    private List<RomItem> displayRomList = new ArrayList<>();
    
    private String currentFilter = "All";
    private String currentSearchQuery = "";

    static {
        System.loadLibrary("main"); 
    }

    public native String getSystemName(String romPath);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_launcher);

        romDir = new File(getFilesDir(), "ROMs");
        if (!romDir.exists()) {
            romDir.mkdirs();
        }

        listView = findViewById(R.id.rom_list_view);
        spinnerFilter = findViewById(R.id.spinner_filter);
        editTextSearch = findViewById(R.id.edit_text_search);
        btnClearSearch = findViewById(R.id.btn_clear_search);
        
        adapter = new RomAdapter(this, displayRomList);
        listView.setAdapter(adapter);

        listView.setOnItemClickListener((parent, view, position, id) -> {
            RomItem item = displayRomList.get(position);
            launchEmulator(item.romFile);
        });

        spinnerFilter.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                currentFilter = parent.getItemAtPosition(position).toString();
                applyFilter();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });

        editTextSearch.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                currentSearchQuery = s.toString().trim().toLowerCase();
                
                btnClearSearch.setVisibility(currentSearchQuery.isEmpty() ? View.GONE : View.VISIBLE);
                applyFilter();
            }

            @Override
            public void afterTextChanged(Editable s) {}
        });

        btnClearSearch.setOnClickListener(v -> editTextSearch.setText(""));

        Button btnAddRom = findViewById(R.id.btnAddRom);
        btnAddRom.setOnClickListener(v -> {
            Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
            intent.setType("*/*");
            startActivityForResult(intent, FILE_PICKER_REQUEST_CODE);
        });

        Button btnOpenBios = findViewById(R.id.btnOpenBios);
        btnOpenBios.setOnClickListener(v -> {
            Intent intent = new Intent(LauncherActivity.this, BiosActivity.class);
            startActivity(intent);
        });

        loadRoms();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == FILE_PICKER_REQUEST_CODE && resultCode == RESULT_OK && data != null) {
            Uri uri = data.getData();
            if (uri != null) {
                copyRomToInternalStorage(uri);
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        loadRoms();
    }

    public void loadRoms() {
        allRomList.clear();
        Set<String> uniqueSystems = new HashSet<>();
        
        File[] files = romDir.listFiles();
        if (files != null) {
            for (File file : files) {
                String name = file.getName().toLowerCase();
                
                if (file.isFile() && !name.endsWith(".bmp") && !name.endsWith(".sav") && !name.endsWith(".state")) {
                    
                    String detectedSystem = getSystemName(file.getAbsolutePath());
                    if (detectedSystem == null || detectedSystem.trim().isEmpty()) {
                        detectedSystem = "Unknown";
                    }
                    
                    File latestScreenshot = getLatestScreenshot(file.getName());
                    long lastChanged = getRomLastModifiedTime(file);
                    
                    allRomList.add(new RomItem(file, latestScreenshot, lastChanged, detectedSystem));
                    uniqueSystems.add(detectedSystem);
                }
            }
        }

        Collections.sort(allRomList, (a, b) -> Long.compare(b.lastModifiedTime, a.lastModifiedTime));

        List<String> spinnerOptions = new ArrayList<>();
        spinnerOptions.add("All");
        List<String> sortedSystems = new ArrayList<>(uniqueSystems);
        Collections.sort(sortedSystems);
        spinnerOptions.addAll(sortedSystems);

        ArrayAdapter<String> spinnerAdapter = new ArrayAdapter<>(
                this,
                android.R.layout.simple_spinner_item,
                spinnerOptions
        );
        spinnerAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerFilter.setAdapter(spinnerAdapter);

        int filterIndex = spinnerOptions.indexOf(currentFilter);
        if (filterIndex >= 0) {
            spinnerFilter.setSelection(filterIndex);
        } else {
            spinnerFilter.setSelection(0); 
            currentFilter = "All";
        }

        applyFilter();
    }

    private void applyFilter() {
        displayRomList.clear();
        
        for (RomItem item : allRomList) {
            boolean matchesSystem = currentFilter.equals("All") || item.systemName.equals(currentFilter);
            boolean matchesSearch = currentSearchQuery.isEmpty() || 
                                    item.romFile.getName().toLowerCase().contains(currentSearchQuery);

            if (matchesSystem && matchesSearch) {
                displayRomList.add(item);
            }
        }
        
        adapter.notifyDataSetChanged();
    }

    private long getRomLastModifiedTime(File romFile) {
        long maxTime = romFile.lastModified();
        String romFileName = romFile.getName();
        int dotIndex = romFileName.lastIndexOf('.');
        String baseName = (dotIndex > 0) ? romFileName.substring(0, dotIndex) : romFileName;

        File savFile = new File(romDir, baseName + ".sav");
        if (savFile.exists()) {
            maxTime = Math.max(maxTime, savFile.lastModified());
        }

        for (int i = 0; i <= 4; i++) {
            File stateFile = new File(romDir, baseName + "." + i + ".state");
            if (stateFile.exists()) {
                maxTime = Math.max(maxTime, stateFile.lastModified());
            }
            File bmpFile = new File(romDir, baseName + "." + i + ".state.bmp");
            if (bmpFile.exists()) {
                maxTime = Math.max(maxTime, bmpFile.lastModified());
            }
        }

        return maxTime;
    }

    private File getLatestScreenshot(String romFileName) {
        int dotIndex = romFileName.lastIndexOf('.');
        if (dotIndex <= 0) return null;

        String baseName = romFileName.substring(0, dotIndex);
        File latestBmp = null;
        long latestTime = 0;

        for (int i = 0; i <= 4; i++) {
            File bmp = new File(romDir, baseName + "." + i + ".state.bmp");
            if (bmp.exists() && bmp.lastModified() > latestTime) {
                latestTime = bmp.lastModified();
                latestBmp = bmp;
            }
        }
        return latestBmp;
    }

    public void deleteRomAndFiles(RomItem item) {
        File romFile = item.romFile;
        String romFileName = romFile.getName();
        
        int dotIndex = romFileName.lastIndexOf('.');
        String baseName = (dotIndex > 0) ? romFileName.substring(0, dotIndex) : romFileName;

        romFile.delete();
        new File(romDir, baseName + ".sav").delete();

        for (int i = 0; i <= 4; i++) {
            new File(romDir, baseName + "." + i + ".state").delete();
            new File(romDir, baseName + "." + i + ".state.bmp").delete();
        }

        Toast.makeText(this, "Deleted: " + romFileName, Toast.LENGTH_SHORT).show();
        loadRoms(); 
    }

    private void copyRomToInternalStorage(Uri uri) {
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
                    filename = "imported_rom_" + System.currentTimeMillis() + ".bin";
                }
            }

            File destFile = new File(romDir, filename);
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
            loadRoms();

        } catch (Exception e) {
            e.printStackTrace();
            Toast.makeText(this, "Failed to copy ROM", Toast.LENGTH_SHORT).show();
        }
    }

    private void launchEmulator(File romFile) {
        Intent intent = new Intent(this, MainActivity.class);
        intent.putExtra("ROM_PATH", romFile.getAbsolutePath());
        startActivity(intent);
    }

    public void showDataMenu(RomItem item) {
        RomDataHelper dataHelper = new RomDataHelper(this, romDir);
        dataHelper.showDataMenu(item);
    }
}
