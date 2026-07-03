package emu;

import android.graphics.Color;
import android.graphics.Typeface;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.Stack;

import org.libsdl.app.R;

public class MenuController {
    private final MainActivity activity;
    private final FrameLayout rootLayout;
    private final LayoutInflater inflater;

    private View scrim;
    private LinearLayout sideMenuContainer;
    private LinearLayout itemsContainer;

    private final CustomMenu rootMenu = new CustomMenu();
    private final Stack<CustomMenu> menuStack = new Stack<>();
    private boolean isMenuOpen = false;

    public MenuController(MainActivity activity, FrameLayout rootLayout) {
        this.activity = activity;
        this.rootLayout = rootLayout;
        this.inflater = LayoutInflater.from(activity);
        
        setupMenuUI();
    }

    public boolean isOpen() {
        return isMenuOpen;
    }

    private void setupMenuUI() {
        // Inflate the floating button
        View fabView = inflater.inflate(R.layout.layout_fab_menu, rootLayout, true);
        fabView.findViewById(R.id.btn_open_menu).setOnClickListener(v -> showRootMenu());

        // Inflate the side drawer
        View menuView = inflater.inflate(R.layout.layout_side_menu, rootLayout, true);
        
        scrim = menuView.findViewById(R.id.menu_scrim);
        sideMenuContainer = menuView.findViewById(R.id.menu_drawer);
        itemsContainer = menuView.findViewById(R.id.menu_items_container);

        scrim.setOnClickListener(v -> closeMenu());

        // Hide drawer off-screen initially
        int drawerWidth = activity.dpToPx(280);
        sideMenuContainer.setTranslationX(-drawerWidth);
    }

    public void showRootMenu() {
        menuStack.clear();
        menuStack.push(rootMenu);
        renderCurrentMenu();
        openMenuUI();
    }

    private void openMenuUI() {
        if (isMenuOpen) return;
        isMenuOpen = true;

        scrim.bringToFront();
        sideMenuContainer.bringToFront();

        scrim.setVisibility(View.VISIBLE);
        scrim.animate().alpha(1f).setDuration(250).start();
        sideMenuContainer.animate().translationX(0).setDuration(250).start();
    }

    public void closeMenu() {
        if (!isMenuOpen) return;
        isMenuOpen = false;
        scrim.animate().alpha(0f).setDuration(250).withEndAction(() -> scrim.setVisibility(View.GONE)).start();
        sideMenuContainer.animate().translationX(-sideMenuContainer.getWidth()).setDuration(250).start();
    }

    private void renderCurrentMenu() {
        itemsContainer.removeAllViews();
        if (menuStack.isEmpty()) return;

        CustomMenu currentMenu = menuStack.peek();

        if (menuStack.size() > 1) {
            itemsContainer.addView(createBackRow());
        }

        for (CustomMenuItem item : currentMenu.items) {
            itemsContainer.addView(createItemRow(item, currentMenu));
        }
    }

    private View createItemRow(CustomMenuItem item, CustomMenu parent) {
        View row = inflater.inflate(R.layout.item_menu_row, itemsContainer, false);
        
        TextView iconView = row.findViewById(R.id.menu_item_icon);
        TextView titleView = row.findViewById(R.id.menu_item_title);
        TextView chevronView = row.findViewById(R.id.menu_item_chevron);

        row.setEnabled(item.enabled);
        row.setAlpha(item.enabled ? 1.0f : 0.4f);
        titleView.setText(item.title);

        if (item.checkable || parent.exclusiveGroups.contains(item.groupId)) {
            boolean isRadio = parent.exclusiveGroups.contains(item.groupId);
            iconView.setTextColor(item.checked ? Color.parseColor("#64FFDA") : Color.parseColor("#666666"));
            if (isRadio) {
                iconView.setText(item.checked ? "●" : "○"); 
            } else {
                iconView.setText(item.checked ? "✔" : ""); 
            }
        }

        if (item.subMenu != null) {
            chevronView.setVisibility(View.VISIBLE);
        }

        row.setOnClickListener(v -> {
            if (!item.enabled) return;
            if (item.subMenu != null) {
                menuStack.push(item.subMenu);
                renderCurrentMenu();
            } else {
                activity.nativeMenuItemClicked(item.itemId);
            }
        });

        return row;
    }

    private View createBackRow() {
        View row = inflater.inflate(R.layout.item_menu_row, itemsContainer, false);
        
        TextView iconView = row.findViewById(R.id.menu_item_icon);
        TextView titleView = row.findViewById(R.id.menu_item_title);

        iconView.setText("❮");
        iconView.setTextColor(Color.LTGRAY);
        titleView.setText("Back");
        titleView.setTextColor(Color.LTGRAY);
        titleView.setTypeface(null, Typeface.BOLD);

        row.setOnClickListener(v -> {
            menuStack.pop();
            renderCurrentMenu();
        });

        // Add a manual divider under the back button
        View divider = new View(activity);
        divider.setBackgroundColor(Color.parseColor("#333333"));
        divider.setLayoutParams(new LinearLayout.LayoutParams(-1, activity.dpToPx(1)));

        LinearLayout container = new LinearLayout(activity);
        container.setOrientation(LinearLayout.VERTICAL);
        container.addView(row);
        container.addView(divider);

        return container;
    }

    private void refreshUIIfNeeded() {
        if (isMenuOpen) {
            renderCurrentMenu();
        }
    }


    private static class CustomMenu {
        List<CustomMenuItem> items = new ArrayList<>();
        Set<Integer> exclusiveGroups = new HashSet<>();
    }

    private static class CustomMenuItem {
        int groupId, itemId;
        String title;
        boolean checkable, checked, enabled = true;
        CustomMenu subMenu;
    }

    public Object getRoot() { return rootMenu; }
    public Object createSubPopup() { return new CustomMenu(); }

    public void addSubmenuLink(Object parent, Object child, String title) {
        CustomMenuItem item = new CustomMenuItem();
        item.title = title;
        item.subMenu = (CustomMenu) child;
        ((CustomMenu) parent).items.add(item);
        refreshUIIfNeeded();
    }

    public void addItem(Object parent, int groupId, int itemId, String title, boolean checkable) {
        CustomMenuItem item = new CustomMenuItem();
        item.groupId = groupId;
        item.itemId = itemId;
        item.title = title;
        item.checkable = checkable;
        ((CustomMenu) parent).items.add(item);
        refreshUIIfNeeded();
    }

    public void setGroupCheckable(Object popup, int groupId) {
        if (popup instanceof CustomMenu) {
            ((CustomMenu) popup).exclusiveGroups.add(groupId);
            refreshUIIfNeeded();
        }
    }

    public void setItemChecked(Object parent, int itemId, boolean checked) {
        if (parent instanceof CustomMenu) {
            for (CustomMenuItem item : ((CustomMenu) parent).items) {
                if (item.itemId == itemId) {
                    item.checkable = true; 
                    item.checked = checked;
                    refreshUIIfNeeded();
                    break;
                }
            }
        }
    }

    public void setItemEnabled(Object parent, int itemId, boolean enabled) {
        if (parent instanceof CustomMenu) {
            for (CustomMenuItem item : ((CustomMenu) parent).items) {
                if (item.itemId == itemId) {
                    item.enabled = enabled;
                    refreshUIIfNeeded();
                    break;
                }
            }
        }
    }

    public void setItemTitle(Object parent, int itemId, String title) {
        if (parent instanceof CustomMenu) {
            for (CustomMenuItem item : ((CustomMenu) parent).items) {
                if (item.itemId == itemId) {
                    item.title = title;
                    refreshUIIfNeeded();
                    break;
                }
            }
        }
    }

    public void destroyAll() {
        closeMenu();
        rootMenu.items.clear();
        rootMenu.exclusiveGroups.clear();
        menuStack.clear();
        refreshUIIfNeeded();
    }
}
