// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.graphics.Bitmap;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.ResourceId;

/**
 * An infobar to tell the user that the current site they are visiting is a PWA.
 */
public class PwaAmbientBadgeInfoBar extends InfoBar {
    private final int mInlineLinkRangeEnd;

    @CalledByNative
    private static InfoBar show(int enumeratedIconId, Bitmap iconBitmap, String messageText) {
        int drawableId = ResourceId.mapToDrawableId(enumeratedIconId);
        return new PwaAmbientBadgeInfoBar(drawableId, iconBitmap, messageText);
    }

    /**
     * Creates the infobar.
     * @param iconDrawableId    Drawable ID corresponding to the icon that the infobar will show.
     * @param iconBitmap        Bitmap of the icon to display in the infobar
     * @param messageText       Message to display in the infobar.
     */
    private PwaAmbientBadgeInfoBar(int iconDrawableId, Bitmap iconBitmap, String messageText) {
        super(iconDrawableId, iconBitmap, messageText);
        mInlineLinkRangeEnd = messageText.length();
    }

    @Override
    public void createContent(InfoBarLayout layout) {
        super.createContent(layout);
        layout.setInlineMessageLink(0, mInlineLinkRangeEnd);
    }
}
