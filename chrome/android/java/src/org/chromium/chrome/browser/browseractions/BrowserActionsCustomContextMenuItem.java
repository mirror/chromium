// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.browseractions;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.support.annotation.IdRes;
import android.support.customtabs.browseractions.BrowserActionItem;

import org.chromium.chrome.browser.contextmenu.ContextMenuItem;

/**
 * A class represents Browser Actions context menu with custom title and icon.
 */
public class BrowserActionsCustomContextMenuItem implements ContextMenuItem {
    @IdRes
    private final int mMenuId;
    private final String mTitle;
    private final Drawable mIcon;
    private final Uri mIconUri;

    /**
     * Constructor to build a custom context menu item from {@link BrowserActionItem}.
     * @param id The {@link IdRes} of the custom context menu item.
     * @param title The title of the custom context menu item.
     * @param icon The icon of the custom context menu item.
     * @param iconUri The {@link Uri} used to access the icon of the custom context menu item.
     */
    BrowserActionsCustomContextMenuItem(@IdRes int id, String title, Drawable icon, Uri iconUri) {
        mMenuId = id;
        mTitle = title;
        mIcon = icon;
        mIconUri = iconUri;
    }

    @Override
    public int getMenuId() {
        return mMenuId;
    }

    @Override
    public String getTitle(Context context) {
        return mTitle;
    }

    @Override
    public Drawable getDrawable(Context context) {
        return mIcon;
    }

    public Uri getIconUri() {
        return mIconUri;
    }
}