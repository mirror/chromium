// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.browseractions;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.support.annotation.IdRes;
import android.support.customtabs.browseractions.BrowserActionItem;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.contextmenu.ContextMenuItem;

/**
 * A class represents Browser Actions context menu with custom title and icon.
 */
public class BrowserActionsCustomContextMenuItem implements ContextMenuItem {
    @IdRes
    private final int mMenuId;
    private final String mTitle;
    private final Uri mIconUri;
    private Drawable mIcon;
    private IconListener mListener;

    /**
     * Interface used to test when {@link Bitmap} from the uri is available.
     */
    @VisibleForTesting
    interface IconListener {
        void onIconPreLoad();
        void onIconShown(Bitmap bm);
    }

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

    /**
     * Set the drawable icon of custom context menu item.
     */
    @VisibleForTesting
    void setDrawable(Drawable drawable) {
        mIcon = drawable;
    }

    /**
     * @return the {@link Uri} used to access the icon of custom context menu item.
     */
    public Uri getIconUri() {
        return mIconUri;
    }

    /**
     * Set the listener used to notify when {@link Bitmap} from the uri is available.
     */
    @VisibleForTesting
    void setIconListener(IconListener listener) {
        mListener = listener;
    }

    /**
     * @return the listener used to notify when {@link Bitmap} from the uri is available.
     */
    @VisibleForTesting
    IconListener getIconListener() {
        return mListener;
    }
}