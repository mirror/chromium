/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.chromium.chrome.browser.widget.bottomsheet.base;

import static android.support.annotation.RestrictTo.Scope.GROUP_ID;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.drawable.Drawable;
import android.support.annotation.NonNull;
import android.support.annotation.RestrictTo;
import android.support.v4.content.ContextCompat;
import android.support.v4.view.ViewCompat;
import android.support.v7.view.menu.MenuItemImpl;
import android.support.v7.view.menu.MenuView;
import android.util.AttributeSet;
import android.widget.LinearLayout;

/**
 * Forked from android.support.design.internal.BottomNavigationItemView.
 */
@RestrictTo(GROUP_ID)
public abstract class BottomNavigationItemView extends LinearLayout implements MenuView.ItemView {
    public static final int INVALID_ITEM_POSITION = -1;

    private static final int[] CHECKED_STATE_SET = {android.R.attr.state_checked};

    private int mItemPosition = INVALID_ITEM_POSITION;
    protected MenuItemImpl mItemData;

    public BottomNavigationItemView(@NonNull Context context) {
        this(context, null);
    }

    public BottomNavigationItemView(@NonNull Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public BottomNavigationItemView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    @Override
    public void initialize(MenuItemImpl itemData, int menuType) {
        mItemData = itemData;
        setCheckable(itemData.isCheckable());
        setChecked(itemData.isChecked());
        setEnabled(itemData.isEnabled());
        setIcon(itemData.getIcon());
        setTitle(itemData.getTitle());
        setId(itemData.getItemId());
    }

    public void setItemPosition(int position) {
        mItemPosition = position;
    }

    public int getItemPosition() {
        return mItemPosition;
    }

    @Override
    public MenuItemImpl getItemData() {
        return mItemData;
    }

    @Override
    public void setCheckable(boolean checkable) {
        refreshDrawableState();
    }

    @Override
    public void setChecked(boolean checked) {
        refreshDrawableState();
    }

    @Override
    public void setEnabled(boolean enabled) {
        super.setEnabled(enabled);
    }

    @Override
    public int[] onCreateDrawableState(final int extraSpace) {
        final int[] drawableState = super.onCreateDrawableState(extraSpace + 1);
        if (mItemData != null && mItemData.isCheckable() && mItemData.isChecked()) {
            mergeDrawableStates(drawableState, CHECKED_STATE_SET);
        }
        return drawableState;
    }

    @Override
    public void setShortcut(boolean showShortcut, char shortcutKey) {}

    @Override
    public boolean prefersCondensedTitle() {
        return false;
    }

    @Override
    public boolean showsIcon() {
        return true;
    }

    public void setItemBackground(int background) {
        Drawable backgroundDrawable =
                background == 0 ? null : ContextCompat.getDrawable(getContext(), background);
        ViewCompat.setBackground(this, backgroundDrawable);
    }

    @Override
    public void setTitle(CharSequence title) {}

    @Override
    public void setIcon(Drawable icon) {}

    /**
     * Assigns the tint of the icon in its various states.
     *
     * @param tint The {@link ColorStateList} representing the tint of the icon in different
     * states.
     */
    public void setIconTint(ColorStateList tint) {}

    /**
     * Assigns the colors of the label in its various states.
     *
     * @param colors The {@link ColorStateList} representing the color of the label in different
     * states.
     */
    public void setTextColors(ColorStateList colors) {}
}