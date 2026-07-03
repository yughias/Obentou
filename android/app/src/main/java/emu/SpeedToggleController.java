package emu;

import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.TextView;

import org.libsdl.app.R;

public class SpeedToggleController {

    private final MainActivity activity;
    private final FrameLayout rootLayout;
    
    private TextView speedButton;

    private int currentSpeedIndex = 2;
    private final int[] speedValues = {-1, 0, 1, 2, 4, 8};
    
    private final String[] speedSymbols = {
        "\u23EA\uFE0E", // Rewind
        "\u25A0\uFE0E", // Stop
        "\u25B6\uFE0E", // x1 Play
        "\u23E9\uFE0E", // x2 Fast Forward
        "4x",           // 4x
        "8x"            // 8x
    };

    public SpeedToggleController(MainActivity activity, FrameLayout rootLayout) {
        this.activity = activity;
        this.rootLayout = rootLayout;
        setupSpeedToggleButton();
    }

    private void setupSpeedToggleButton() {
        LayoutInflater inflater = LayoutInflater.from(activity);
        View containerView = inflater.inflate(R.layout.layout_speed_toggle, rootLayout, true);
        
        speedButton = containerView.findViewById(R.id.btn_speed_toggle);
        speedButton.setText(speedSymbols[currentSpeedIndex]);

        speedButton.setOnTouchListener(new View.OnTouchListener() {
            private float startX;
            private float startY;
            private final int SWIPE_THRESHOLD = activity.dpToPx(30);

            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getActionMasked()) {
                    case MotionEvent.ACTION_DOWN:
                        startX = event.getRawX();
                        startY = event.getRawY();
                        
                        v.setPressed(true); 
                        updateButtonLabel(true);
                        speedButton.animate().scaleX(1.15f).scaleY(1.15f).setDuration(100).start();
                        break;
                        
                    case MotionEvent.ACTION_UP:
                        float diffX = event.getRawX() - startX;
                        float diffY = event.getRawY() - startY;

                        if (Math.abs(diffX) > Math.abs(diffY) && Math.abs(diffX) > SWIPE_THRESHOLD) {
                            int oldIndex = currentSpeedIndex;
                            
                            if (diffX > 0) {
                                currentSpeedIndex = Math.min(speedValues.length - 1, currentSpeedIndex + 1);
                            } else {
                                currentSpeedIndex = Math.max(0, currentSpeedIndex - 1);
                            }
                            
                            if (oldIndex != currentSpeedIndex) {
                                activity.adjustSpeed(speedValues[currentSpeedIndex]); 
                            }
                        }
                        
                    case MotionEvent.ACTION_CANCEL:
                        v.setPressed(false);
                        updateButtonLabel(false);
                        speedButton.animate().scaleX(1.0f).scaleY(1.0f).setDuration(150).start();
                        break;
                }
                return true;
            }
        });
    }

    private void updateButtonLabel(boolean isTouching) {
        String text = speedSymbols[currentSpeedIndex];
        if (isTouching) {
            if (currentSpeedIndex != 0)
                text = "\u276E  " + text;
            if (currentSpeedIndex != speedSymbols.length - 1)
                text = text + "  \u276F";
        }
        speedButton.setText(text); 
    }
}
