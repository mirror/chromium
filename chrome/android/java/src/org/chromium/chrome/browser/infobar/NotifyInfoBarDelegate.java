// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.support.annotation.DrawableRes;

import org.chromium.chrome.browser.ResourceId;

/**
 * Defines the appearance and callbacks for {@link NotifyInfoBar}.
 */
class NotifyInfoBarDelegate {
    private final long mNativeDelegate;

    public NotifyInfoBarDelegate(long nativeDelegate) {
        mNativeDelegate = nativeDelegate;
    }

    /** The full description of the notification, visible when the infobar is expanded. */
    public String getDescription() {
        return nativeGetDescription(mNativeDelegate);
    }

    /** The short description of the notification, visible when the infobar is collapsed. */
    public String getShortDescription() {
        return nativeGetShortDescription(mNativeDelegate);
    }

    /** The text for the link displayed when the infobar is expanded. */
    public String getFeaturedLinkText() {
        return nativeGetFeaturedLinkText(mNativeDelegate);
    }

    /** The icon to show in this infobar. */
    @DrawableRes
    public int getIconResourceId() {
        return ResourceId.mapToDrawableId(nativeGetEnumeratedIcon(mNativeDelegate));
    }

    /** Callback called when the featured link is tapped. */
    public void onLinkTapped() {
        nativeOnLinkTapped(mNativeDelegate);
    }

    private native String nativeGetDescription(long nativeNotifyInfoBarDelegateAndroid);
    private native String nativeGetShortDescription(long nativeNotifyInfoBarDelegateAndroid);
    private native String nativeGetFeaturedLinkText(long nativeNotifyInfoBarDelegateAndroid);
    private native int nativeGetEnumeratedIcon(long nativeNotifyInfoBarDelegateAndroid);
    private native void nativeOnLinkTapped(long nativeNotifyInfoBarDelegateAndroid);
}
