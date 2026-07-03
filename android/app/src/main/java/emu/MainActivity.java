package emu;

import org.libsdl.app.SDLActivity;

import android.os.Bundle;
import android.view.KeyEvent;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.Toast;

public class MainActivity extends SDLActivity {

    private FrameLayout widgetLayer; // Bottom layer for draggable images
    private FrameLayout uiLayer;     // Top layer for menus and buttons
    
    private MenuController menuController;
    private WidgetController widgetController;
    private SpeedToggleController speedController;

    private long backPressedTime = 0;
    private Toast backToast;
    private boolean isExiting = false;

    @Override
    protected String[] getArguments() {
        String romPath = getIntent().getStringExtra("ROM_PATH");
        if (romPath != null) {
            return new String[]{ romPath }; 
        }
        return super.getArguments();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        widgetLayer = new FrameLayout(this);
        widgetLayer.setClickable(false); 
        widgetLayer.setFocusable(false);
        mLayout.addView(widgetLayer, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 
                ViewGroup.LayoutParams.MATCH_PARENT));

        uiLayer = new FrameLayout(this);
        uiLayer.setClickable(false); 
        uiLayer.setFocusable(false);
        // Force the UI layer to always sit higher on the Z-axis than any elevated widget
        uiLayer.setElevation(dpToPx(1001)); 
        mLayout.addView(uiLayer, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 
                ViewGroup.LayoutParams.MATCH_PARENT));

        widgetController = new WidgetController(this, widgetLayer);
        
        menuController = new MenuController(this, uiLayer);
        speedController = new SpeedToggleController(this, uiLayer);
    }

    @Override
    protected void onDestroy() {
        if (menuController != null) menuController.destroyAll();
        super.onDestroy();
    }

    // double tap to exit and back to close menu
    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (event.getKeyCode() == KeyEvent.KEYCODE_BACK) {
            
            if (menuController != null && menuController.isOpen()) {
                if (event.getAction() == KeyEvent.ACTION_UP) {
                    menuController.closeMenu();
                }
                return true; 
            }

            if (isExiting) {
                return super.dispatchKeyEvent(event);
            }

            if (event.getAction() == KeyEvent.ACTION_DOWN) {
                if (System.currentTimeMillis() - backPressedTime < 2000) {
                    isExiting = true;
                    if (backToast != null) backToast.cancel();
                    return super.dispatchKeyEvent(event);
                } else {
                    backPressedTime = System.currentTimeMillis();
                    backToast = Toast.makeText(this, "Press BACK again to exit", Toast.LENGTH_SHORT);
                    backToast.show();
                    return true; 
                }
            } else if (event.getAction() == KeyEvent.ACTION_UP) {
                return true;
            }
        }
        
        return super.dispatchKeyEvent(event);
    }

    public int dpToPx(int dp) {
        return (int) (dp * getResources().getDisplayMetrics().density);
    }

    public Object javaMenuGetRoot() { return menuController.getRoot(); }
    public Object javaMenuCreateSubPopup() { return menuController.createSubPopup(); }
    public void javaMenuAddSubmenuLink(Object parent, Object child, String title) { runOnUiThread(() -> menuController.addSubmenuLink(parent, child, title)); }
    public void javaMenuAddItem(Object parent, int groupId, int itemId, String title, boolean checkable) { runOnUiThread(() -> menuController.addItem(parent, groupId, itemId, title, checkable)); }
    public void javaMenuSetGroupCheckable(Object popup, int groupId) { runOnUiThread(() -> menuController.setGroupCheckable(popup, groupId)); }
    public void javaMenuSetItemChecked(Object parent, int itemId, boolean checked) { runOnUiThread(() -> menuController.setItemChecked(parent, itemId, checked)); }
    public void javaMenuSetItemEnabled(Object parent, int itemId, boolean enabled) { runOnUiThread(() -> menuController.setItemEnabled(parent, itemId, enabled)); }
    public void javaMenuSetItemTitle(Object parent, int itemId, String title) { runOnUiThread(() -> menuController.setItemTitle(parent, itemId, title)); }
    public void javaMenuDestroyAll() { runOnUiThread(() -> menuController.destroyAll()); }

    public void javaWidgetCreate(int id, String name, int w, int h) { widgetController.create(id, name, w, h); }
    public void javaWidgetUpdate(int id, int[] pixels, int w, int h) { widgetController.update(id, pixels, w, h); }
    public void javaWidgetDestroy(int id) { widgetController.destroy(id); }

    public native void nativeMenuItemClicked(int id);
    public native void nativeWidgetClosed(int id);
    public native void adjustSpeed(int speedIndex);
}
