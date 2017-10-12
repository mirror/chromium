// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.bottomsheet;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.drawable.Drawable;
import android.support.annotation.NonNull;
import android.support.v4.graphics.drawable.DrawableCompat;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.widget.ImageView;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.bottomsheet.base.BottomNavigationItemView;

/**
 * An implementation of the forked {@link BottomNavigationItemView} specifically for the Chrome Home
 * bottom navigation menu.
 */
public class BottomSheetNavigationItemView extends BottomNavigationItemView {
    private boolean mLabelHidden;
    private ImageView mIcon;
    private TextView mLabel;
    private ColorStateList mIconTint;

    public BottomSheetNavigationItemView(@NonNull Context context) {
        super(context);

        LayoutInflater.from(context).inflate(R.layout.bottom_sheet_nav_menu_item, this, true);
        setBackgroundResource(
                android.support.design.R.drawable.design_bottom_navigation_item_background);
        mIcon = (ImageView) findViewById(R.id.icon);
        mLabel = (TextView) findViewById(R.id.label);
    }

    public BottomSheetNavigationItemView(@NonNull Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public BottomSheetNavigationItemView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    @Override
    public void setTitle(CharSequence title) {
        mLabel.setText(title);
    }

    @Override
    public void setIcon(Drawable icon) {
        if (icon != null) {
            Drawable.ConstantState state = icon.getConstantState();
            icon = DrawableCompat.wrap(state == null ? icon : state.newDrawable()).mutate();
            DrawableCompat.setTintList(icon, mIconTint);
        }
        mIcon.setImageDrawable(icon);
    }

    @Override
    public void setEnabled(boolean enabled) {
        mLabel.setEnabled(enabled);
        mIcon.setEnabled(enabled);
    }

    /**
     * Hides the label below the menu item's icon. Also adds a content description for accessibility
     * purposes since there's no label to read any more.
     */
    public void hideLabel() {
        if (mLabel == null || mLabelHidden) return;
        mLabelHidden = true;
        mLabel.setVisibility(GONE);
        setContentDescription(mItemData.getTitle());
    }

    @Override
    public void setIconTintList(ColorStateList tint) {
        mIconTint = tint;
        if (mItemData != null) {
            // Update the icon so that the tint takes effect
            setIcon(mItemData.getIcon());
        }
    }

    @Override
    public void setTextColor(ColorStateList color) {
        mLabel.setTextColor(color);
    }
}