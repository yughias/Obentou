package launcher;

import org.libsdl.app.R;

import android.app.AlertDialog;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;
import androidx.core.content.FileProvider;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class RomDataHelper {

    private final LauncherActivity activity;
    private final File romDir;

    public RomDataHelper(LauncherActivity activity, File romDir) {
        this.activity = activity;
        this.romDir = romDir;
    }

    public void showDataMenu(RomItem item) {
        List<File> dataFiles = getAssociatedFiles(item.romFile);
        
        if (dataFiles.isEmpty()) {
            Toast.makeText(activity, "No save data found for this ROM.", Toast.LENGTH_SHORT).show();
            return;
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(activity);
        builder.setTitle("Data: " + item.romFile.getName());

        ArrayAdapter<File> dataAdapter = new ArrayAdapter<File>(activity, R.layout.item_data_file, dataFiles) {
            @Override
            public View getView(int position, View convertView, ViewGroup parent) {
                if (convertView == null) {
                    convertView = LayoutInflater.from(getContext()).inflate(R.layout.item_data_file, parent, false);
                }

                File file = getItem(position);
                TextView tvFileName = convertView.findViewById(R.id.tvDataFileName);
                ImageView ivImage = convertView.findViewById(R.id.ivDataImage);
                ImageButton btnShare = convertView.findViewById(R.id.btnShareData);
                ImageButton btnDelete = convertView.findViewById(R.id.btnDeleteData);

                if (file != null) {
                    tvFileName.setText(file.getName());

                    String fileName = file.getName();
                    File thumbnailFile = null;
                    if (fileName.toLowerCase().endsWith(".bmp")) {
                        thumbnailFile = file;
                    }

                    if (thumbnailFile != null && thumbnailFile.exists()) {
                        Bitmap bmp = BitmapFactory.decodeFile(thumbnailFile.getAbsolutePath());
                        ivImage.setImageBitmap(bmp);
                        ivImage.setVisibility(View.VISIBLE);
                    } else {
                        ivImage.setImageBitmap(null);
                        ivImage.setVisibility(View.INVISIBLE);
                    }

                    btnShare.setOnClickListener(v -> shareFile(file));

                    btnDelete.setOnClickListener(v -> {
                        new AlertDialog.Builder(getContext())
                            .setMessage("Delete " + file.getName() + "?")
                            .setPositiveButton("Yes", (dialog, which) -> {
                                file.delete();
                                remove(file); 
                                notifyDataSetChanged();
                                activity.loadRoms(); // Triggers a UI refresh in the main activity
                            })
                            .setNegativeButton("No", null)
                            .show();
                    });
                }
                return convertView;
            }
        };

        builder.setAdapter(dataAdapter, null);
        builder.setPositiveButton("Close", null);
        builder.show();
    }

    private List<File> getAssociatedFiles(File romFile) {
        List<File> associatedFiles = new ArrayList<>();
        String romFileName = romFile.getName();
        int dotIndex = romFileName.lastIndexOf('.');
        String baseName = (dotIndex > 0) ? romFileName.substring(0, dotIndex) : romFileName;

        File[] files = romDir.listFiles((dir, name) -> {
            return name.startsWith(baseName) && !name.equals(romFileName);
        });

        if (files != null) {
            for (File f : files) {
                associatedFiles.add(f);
            }
        }
        
        Collections.sort(associatedFiles, (f1, f2) -> f1.getName().compareToIgnoreCase(f2.getName()));
        return associatedFiles;
    }

    private void shareFile(File file) {
        try {
            Uri fileUri = FileProvider.getUriForFile(
                    activity, 
                    activity.getApplicationContext().getPackageName() + ".provider", 
                    file
            );

            Intent intent = new Intent(Intent.ACTION_SEND);
            
            if (file.getName().toLowerCase().endsWith(".bmp")) {
                intent.setType("image/bmp");
            } else {
                intent.setType("application/octet-stream");
            }
            
            intent.putExtra(Intent.EXTRA_STREAM, fileUri);
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);

            activity.startActivity(Intent.createChooser(intent, "Share Data File"));
        } catch (IllegalArgumentException e) {
            Toast.makeText(activity, "Failed to share file: FileProvider not configured.", Toast.LENGTH_LONG).show();
        }
    }
}
