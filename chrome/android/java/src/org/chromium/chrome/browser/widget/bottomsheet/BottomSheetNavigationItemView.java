// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.bottomsheet;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.drawable.Drawable;
import android.support.annotation.NonNull;
import android.support.v4.graphics.drawable.DrawableCompat;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.widget.ImageView;
import android.widget.LinearLayout;
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
        super(context, null, 0);

        LayoutInflater.from(context).inflate(R.layout.bottom_sheet_nav_menu_item, this, true);
        setOrientation(LinearLayout.VERTICAL);
        setGravity(Gravity.CENTER);
        setBackgroundResource(
                android.support.design.R.drawable.design_bottom_navigation_item_background);
        mIcon = (ImageView) findViewById(R.id.icon);
        mLabel = (TextView) findViewById(R.id.label);
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
    public void setIconTint(ColorStateList tint) {
        mIconTint = tint;
        if (mItemData != null) {
            // Update the icon so that the tint takes effect
            setIcon(mItemData.getIcon());
        }
    }

    @Override
    public void setTextColors(ColorStateList colors) {
        mLabel.setTextColor(colors);
    }
}