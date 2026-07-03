package launcher;

import org.libsdl.app.R;

import android.app.AlertDialog;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.BitmapFactory;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.List;

public class RomAdapter extends ArrayAdapter<RomItem> {
    
    public RomAdapter(Context context, List<RomItem> items) {
        super(context, 0, items);
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        RomItem item = getItem(position);
        
        if (convertView == null) {
            convertView = LayoutInflater.from(getContext()).inflate(R.layout.item_rom, parent, false);
        }

        TextView tvTitle = convertView.findViewById(R.id.tvRomTitle);
        TextView tvSystem = convertView.findViewById(R.id.tvRomSystem); 
        ImageView ivScreenshot = convertView.findViewById(R.id.ivScreenshot);
        ImageButton btnDelete = convertView.findViewById(R.id.btnDeleteRom);

        if (item != null) {
            tvTitle.setText(item.romFile.getName());
            
            if (item.systemName != null) {
                tvSystem.setText(item.systemName);
                tvSystem.setVisibility(View.VISIBLE);
            } else {
                tvSystem.setVisibility(View.GONE);
            }
            
            if (item.screenshotFile != null && item.screenshotFile.exists()) {
                Bitmap bmp = BitmapFactory.decodeFile(item.screenshotFile.getAbsolutePath());
                // wrap bmp into a BitmapDrawable to disable linear filtering
                BitmapDrawable drawable = new BitmapDrawable(ivScreenshot.getResources(), bmp);
                drawable.setFilterBitmap(false);
                drawable.setAntiAlias(false);
                ivScreenshot.setImageDrawable(drawable);
                ivScreenshot.setVisibility(View.VISIBLE);
            } else {
                ivScreenshot.setImageBitmap(null);
            }

            // Delete Button Logic
            btnDelete.setOnClickListener(v -> {
                new AlertDialog.Builder(getContext())
                        .setTitle("Delete ROM")
                        .setMessage("Are you sure you want to delete '" + item.romFile.getName() + "'?")
                        .setPositiveButton("Delete", (dialog, which) -> {
                            if (getContext() instanceof LauncherActivity) {
                                ((LauncherActivity) getContext()).deleteRomAndFiles(item);
                            }
                        })
                        .setNegativeButton("Cancel", null)
                        .show();
            });

            // Show Data Menu Logic
            ImageButton btnShowData = convertView.findViewById(R.id.btnShowData);
            btnShowData.setOnClickListener(v -> {
                if (getContext() instanceof LauncherActivity) {
                    ((LauncherActivity) getContext()).showDataMenu(item);
                }
            });
        }
        return convertView;
    }
}
