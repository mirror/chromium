// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.support.annotation.DrawableRes;

import org.chromium.chrome.browser.ResourceId;

/** Implementation of {@link NotifyInfoBarDelegate} that pulls data from C++. */
class NotifyInfoBarDelegateBridge implements NotifyInfoBarDelegate {
    private final long mNativeDelegate;

    public NotifyInfoBarDelegateBridge(long nativeDelegate) {
        mNativeDelegate = nativeDelegate;
    }

    @Override
    public String getDescription() {
        return nativeGetDescription(mNativeDelegate);
    }

    @Override
    public String getShortDescription() {
        return nativeGetShortDescription(mNativeDelegate);
    }

    @Override
    public String getFeaturedLinkText() {
        return nativeGetFeaturedLinkText(mNativeDelegate);
    }

    @Override
    @DrawableRes
    public int getIconResourceId() {
        return ResourceId.mapToDrawableId(nativeGetEnumeratedIcon(mNativeDelegate));
    }

    @Override
    public void onLinkTapped() {
        nativeOnLinkTapped(mNativeDelegate);
    }

    private native String nativeGetDescription(long nativeNotifyInfoBarDelegateBridge);
    private native String nativeGetShortDescription(long nativeNotifyInfoBarDelegateBridge);
    private native String nativeGetFeaturedLinkText(long nativeNotifyInfoBarDelegateBridge);
    private native int nativeGetEnumeratedIcon(long nativeNotifyInfoBarDelegateBridge);
    private native void nativeOnLinkTapped(long nativeNotifyInfoBarDelegateBridge);
}
