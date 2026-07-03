package emu;

import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

public class NativeWidget {
    private final MainActivity activity;
    private final WidgetController controller;
    private final int id;
    private final LinearLayout container;
    private final ImageView imageView;
    
    private Bitmap bitmap;
    private int baseW, baseH;
    private float scale = 1.0f;
    private final int MIN_SIZE_PX;
    private boolean userHasResized = false; 

    public NativeWidget(MainActivity activity, WidgetController controller, int id, String name, int w, int h) {
        this.activity = activity;
        this.controller = controller;
        this.id = id;
        this.baseW = w;
        this.baseH = h;
        this.MIN_SIZE_PX = activity.dpToPx(48);

        if (w > 0 && h > 0) {
            calculateIdealScale(w, h);
        }

        this.container = createContainer();
        View header = createHeader(name);
        FrameLayout imageFrame = new FrameLayout(activity);
        
        this.imageView = new ImageView(activity);
        this.imageView.setScaleType(ImageView.ScaleType.FIT_XY);
        applyScaleLayout();

        View resizeHandle = createResizeHandle();

        imageFrame.addView(this.imageView);
        imageFrame.addView(resizeHandle, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.BOTTOM | Gravity.END));

        this.container.addView(header);
        this.container.addView(imageFrame);

        setupGestureListeners();
    }

    public View getView() { return container; }

    public void updatePixels(int[] pixels, int w, int h) {
        if (w <= 0 || h <= 0) return; 

        if (bitmap == null || baseW != w || baseH != h) {
            
            if (userHasResized && baseW > 0) {
                float currentPhysicalWidth = baseW * scale;
                scale = currentPhysicalWidth / w; 
            } else {
                calculateIdealScale(w, h);
            }
            
            float minScaleW = (float) MIN_SIZE_PX / w;
            float minScaleH = (float) MIN_SIZE_PX / h;
            scale = Math.max(scale, Math.min(minScaleW, minScaleH));

            bitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
            baseW = w;
            baseH = h;
            applyScaleLayout();
        }

        bitmap.setPixels(pixels, 0, w, 0, 0, w, h);

        BitmapDrawable drawable = new BitmapDrawable(container.getResources(), bitmap);
        drawable.setFilterBitmap(false);
        imageView.setImageDrawable(drawable);
    }

    private void calculateIdealScale(int w, int h) {
        float minScaleW = (float) MIN_SIZE_PX / w;
        float minScaleH = (float) MIN_SIZE_PX / h;
        float baseMinScale = Math.max(minScaleW, minScaleH);

        int screenW = activity.getResources().getDisplayMetrics().widthPixels;
        int screenH = activity.getResources().getDisplayMetrics().heightPixels;
        
        float maxScaleW = (float) (screenW * 0.8f) / w;
        float maxScaleH = (float) (screenH * 0.8f) / h;
        float maxScale = Math.min(maxScaleW, maxScaleH);

        if (w > screenW * 0.8f || h > screenH * 0.8f) {
            this.scale = maxScale;
        } else {
            this.scale = Math.max(1.0f, baseMinScale);
        }
    }

    private LinearLayout createContainer() {
        LinearLayout layout = new LinearLayout(activity);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setBackgroundColor(Color.DKGRAY);
        layout.setElevation(activity.dpToPx(6));
        return layout;
    }

    private View createHeader(String name) {
        LinearLayout header = new LinearLayout(activity);
        header.setOrientation(LinearLayout.HORIZONTAL);
        header.setBackgroundColor(Color.parseColor("#333333"));

        TextView title = new TextView(activity);
        title.setText(name);
        title.setTextColor(Color.WHITE);
        title.setPadding(20, 15, 20, 15);
        header.addView(title, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));

        TextView closeBtn = new TextView(activity);
        closeBtn.setText("\u2715");
        closeBtn.setTextColor(Color.WHITE);
        closeBtn.setPadding(20, 15, 25, 15);
        closeBtn.setOnClickListener(v -> {
            controller.destroy(id);
        });
        header.addView(closeBtn);

        float originalElevation = container.getElevation();

        header.setOnTouchListener(new View.OnTouchListener() {
            float dX, dY;

            @Override
            public boolean onTouch(View view, MotionEvent event) {
                switch (event.getActionMasked()) {
                    case MotionEvent.ACTION_DOWN:
                        dX = container.getX() - event.getRawX();
                        dY = container.getY() - event.getRawY();

                        container.bringToFront();
                        container.setElevation(1000f);
                        return true;

                    case MotionEvent.ACTION_MOVE:
                        container.setX(event.getRawX() + dX);
                        container.setY(event.getRawY() + dY);
                        return true;

                    case MotionEvent.ACTION_UP:
                    case MotionEvent.ACTION_CANCEL:
                        container.setElevation(originalElevation);
                        return true;
                }
                return false;
            }
        });

        return header;
    }

    private View createResizeHandle() {
        TextView handle = new TextView(activity);
        handle.setText("\u25E2");
        handle.setTextColor(Color.LTGRAY);
        handle.setTextSize(18f);
        handle.setIncludeFontPadding(false);
        handle.setPadding(80, 80, 0, 0); 
        handle.setShadowLayer(4f, 0f, 0f, Color.BLACK);

        handle.setOnTouchListener(new View.OnTouchListener() {
            float startX, startScale;
            @Override public boolean onTouch(View view, MotionEvent event) {
                switch (event.getActionMasked()) {
                    case MotionEvent.ACTION_DOWN:
                        startX = event.getRawX();
                        startScale = scale;
                        return true;
                    case MotionEvent.ACTION_MOVE:
                        float dx = event.getRawX() - startX;
                        updateScale(((baseW * startScale) + dx) / baseW);
                        return true;
                }
                return false;
            }
        });
        return handle;
    }

    private void setupGestureListeners() {
        ScaleGestureDetector scaleDetector = new ScaleGestureDetector(activity, new ScaleGestureDetector.SimpleOnScaleGestureListener() {
            @Override public boolean onScale(ScaleGestureDetector detector) {
                updateScale(scale * detector.getScaleFactor());
                return true;
            }
        });

        imageView.setOnTouchListener((v, event) -> {
            scaleDetector.onTouchEvent(event);
            return true;
        });
    }

    private void updateScale(float newScale) {
        userHasResized = true; 
        
        float minScaleW = (float) MIN_SIZE_PX / baseW;
        float minScaleH = (float) MIN_SIZE_PX / baseH;
        this.scale = Math.max(Math.min(minScaleW, minScaleH), newScale);
        applyScaleLayout();
    }

    private void applyScaleLayout() {
        int scaledW = Math.max(1, (int) (baseW * scale));
        int scaledH = Math.max(1, (int) (baseH * scale));
        imageView.setLayoutParams(new FrameLayout.LayoutParams(scaledW, scaledH));
    }
}
