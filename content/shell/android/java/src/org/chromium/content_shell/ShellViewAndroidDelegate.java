// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_shell;

import android.graphics.Bitmap;
import android.view.ViewGroup;

import org.chromium.blink_public.web.WebCursorInfoType;
import org.chromium.ui.base.ViewAndroidDelegate;

import java.util.concurrent.TimeoutException;

/**
 * Implementation of the abstract class {@link ViewAndroidDelegate} for content shell.
 * Extended for testing.
 */
public class ShellViewAndroidDelegate extends ViewAndroidDelegate {
    /**
     * An interface delegates a {@link CallbackHelper} for cursor update.
     */
    public interface OnCursorUpdateHelper {
        /**
         * See {@link CallbackHelper#getCallCount}.
         */
        int getCallCount();

        /**
         * Get the last pointer type of {@link notifyCalled} method.
         */
        int getPointerType();

        /**
         * Record the last notifyCalled pointer type, see more {@link CallbackHelper#notifyCalled}.
         * @param type The pointer type of the notifyCalled.
         */
        void notifyCalled(int type);

        /**
         * See {@link CallbackHelper#waitForCallback}
         */
        void waitForCallback(int currentCallCount) throws InterruptedException, TimeoutException;
    }

    private final ViewGroup mContainerView;
    private OnCursorUpdateHelper mOnCursorUpdateHelper;

    public ShellViewAndroidDelegate(ViewGroup containerView) {
        mContainerView = containerView;
    }

    @Override
    public ViewGroup getContainerView() {
        return mContainerView;
    }

    public void setOnCursorUpdateHelper(OnCursorUpdateHelper helper) {
        mOnCursorUpdateHelper = helper;
    }

    public OnCursorUpdateHelper getOnCursorUpdateHelper() {
        return mOnCursorUpdateHelper;
    }

    @Override
    public void onCursorChangedToCustom(Bitmap customCursorBitmap, int hotspotX, int hotspotY) {
        super.onCursorChangedToCustom(customCursorBitmap, hotspotX, hotspotY);
        if (mOnCursorUpdateHelper != null) {
            mOnCursorUpdateHelper.notifyCalled(WebCursorInfoType.CUSTOM);
        }
    }

    @Override
    public void onCursorChanged(int cursorType) {
        super.onCursorChanged(cursorType);
        if (mOnCursorUpdateHelper != null) {
            mOnCursorUpdateHelper.notifyCalled(cursorType);
        }
    }
}
