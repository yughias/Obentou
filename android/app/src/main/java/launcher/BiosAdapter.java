package launcher;

import org.libsdl.app.R;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.TextView;
import java.util.List;

public class BiosAdapter extends ArrayAdapter<BiosItem> {

    public BiosAdapter(Context context, List<BiosItem> items) {
        super(context, 0, items);
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        BiosItem item = getItem(position);

        if (convertView == null) {
            convertView = LayoutInflater.from(getContext()).inflate(R.layout.item_bios, parent, false);
        }

        TextView tvTitle = convertView.findViewById(R.id.tvBiosTitle);
        TextView tvAssignedSystem = convertView.findViewById(R.id.tvAssignedSystem);
        ImageButton btnDelete = convertView.findViewById(R.id.btnDeleteBios);
        ImageButton btnInfo = convertView.findViewById(R.id.btnBiosInfo);
        Button btnSetSystem = convertView.findViewById(R.id.btnSetSystem);

        if (item != null) {
            tvTitle.setText(item.biosFile.getName());

            if (item.assignedSystem != null && !item.assignedSystem.trim().isEmpty()) {
                tvAssignedSystem.setText("Default for: " + item.assignedSystem);
                tvAssignedSystem.setVisibility(View.VISIBLE);
            } else {
                tvAssignedSystem.setVisibility(View.GONE);
            }

            btnDelete.setOnClickListener(v -> {
                if (getContext() instanceof BiosActivity) {
                    ((BiosActivity) getContext()).deleteBiosFile(item);
                }
            });

            btnInfo.setOnClickListener(v -> {
                if (getContext() instanceof BiosActivity) {
                    ((BiosActivity) getContext()).showBiosInfo(item);
                }
            });

            btnSetSystem.setOnClickListener(v -> {
                if (getContext() instanceof BiosActivity) {
                    ((BiosActivity) getContext()).showSystemSelectionMenu(item);
                }
            });
        }
        return convertView;
    }
}
