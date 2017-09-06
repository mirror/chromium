// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.interventions;

import android.support.annotation.DrawableRes;

import org.chromium.chrome.browser.ResourceId;

/** Implementation of {@link FramebustBlockMessageDelegate} that pulls data from C++. */
public class FramebustBlockMessageDelegateBridge implements FramebustBlockMessageDelegate {
    private final long mNativeDelegate;

    public FramebustBlockMessageDelegateBridge(long nativeDelegate) {
        mNativeDelegate = nativeDelegate;
    }

    @Override
    public String getMessage() {
        return nativeGetMessage(mNativeDelegate);
    }

    @Override
    public String getShortMessage() {
        return nativeGetShortMessage(mNativeDelegate);
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

    private native String nativeGetMessage(long nativeFramebustBlockMessageDelegateBridge);
    private native String nativeGetShortMessage(long nativeFramebustBlockMessageDelegateBridge);
    private native String nativeGetFeaturedLinkText(long nativeFramebustBlockMessageDelegateBridge);
    private native int nativeGetEnumeratedIcon(long nativeFramebustBlockMessageDelegateBridge);
    private native void nativeOnLinkTapped(long nativeFramebustBlockMessageDelegateBridge);
}
