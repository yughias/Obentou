package emu;

import android.view.ViewGroup;
import android.widget.FrameLayout;

import java.util.HashMap;
import java.util.Map;

public class WidgetController {
    private final MainActivity activity;
    private final ViewGroup rootLayout;
    private final Map<Integer, NativeWidget> widgets = new HashMap<>();

    public WidgetController(MainActivity activity, ViewGroup rootLayout) {
        this.activity = activity;
        this.rootLayout = rootLayout;
    }

    public void create(int id, String name, int w, int h) {
        activity.runOnUiThread(() -> {
            NativeWidget widget = new NativeWidget(activity, this, id, name, w, h);
            widgets.put(id, widget);
            
            FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            params.leftMargin = 50 + (id * 40);
            params.topMargin = 150 + (id * 40);

            rootLayout.addView(widget.getView(), params);
        });
    }

    public void update(int id, int[] pixels, int w, int h) {
        activity.runOnUiThread(() -> {
            NativeWidget widget = widgets.get(id);
            if (widget != null) widget.updatePixels(pixels, w, h);
        });
    }

    public void destroy(int id) {
        activity.runOnUiThread(() -> {
            NativeWidget widget = widgets.remove(id);
            if (widget != null) {
                rootLayout.removeView(widget.getView());
                activity.nativeWidgetClosed(id);
            }
        });
    }
}
