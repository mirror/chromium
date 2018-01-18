// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.view.KeyEvent;
import android.view.ViewGroup;

import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content.browser.PopupZoomer.OnTapListener;
import org.chromium.content.browser.PopupZoomer.OnVisibilityChangedListener;
import org.chromium.content.browser.webcontents.WebContentsUserData;
import org.chromium.content.browser.webcontents.WebContentsUserData.UserDataFactory;
import org.chromium.content_public.browser.ImeEventObserver;
import org.chromium.content_public.browser.WebContents;

/**
 * Class that handles tap disambiguation feature.  When a tap lands ambiguously
 * between two tiny touch targets (usually links) on a desktop site viewed on a phone,
 * a magnified view of the content is shown, the screen is grayed out and the user
 * must re-tap the magnified content in order to clarify their intent.
 */
@JNINamespace("content")
public class TapDisambiguator implements ImeEventObserver {
    private final WebContents mWebContents;
    private PopupZoomer mView;
    private boolean mInitialized;
    private long mNativeTapDisambiguator;

    private static final class UserDataFactoryLazyHolder {
        private static final UserDataFactory<TapDisambiguator> INSTANCE = TapDisambiguator::new;
    }

    public static TapDisambiguator create(
            Context context, WebContents webContents, ViewGroup containerView) {
        TapDisambiguator popup = WebContentsUserData.fromWebContents(
                webContents, TapDisambiguator.class, UserDataFactoryLazyHolder.INSTANCE);
        assert popup != null && !popup.initialized();
        popup.init(context, containerView);
        return popup;
    }

    public static TapDisambiguator fromWebContents(WebContents webContents) {
        return WebContentsUserData.fromWebContents(webContents, TapDisambiguator.class, null);
    }

    /**
     * Creates TapDisambiguation instance.
     * @param webContents WebContents instance with which this TapDisambiguator is associated.
     */
    public TapDisambiguator(WebContents webContents) {
        mWebContents = webContents;
    }

    private void init(Context context, ViewGroup containerView) {
        mView = new PopupZoomer(context, containerView);
        // OnVisibilityChangedListener, OnTapListener can only be used to add and remove views
        // from the container view at creation.
        OnVisibilityChangedListener listener = new OnVisibilityChangedListener() {
            @Override
            public void onPopupZoomerShown(final PopupZoomer zoomer) {
                containerView.post(new Runnable() {
                    @Override
                    public void run() {
                        if (containerView.indexOfChild(zoomer) == -1) {
                            containerView.addView(zoomer);
                        }
                    }
                });
            }

            @Override
            public void onPopupZoomerHidden(final PopupZoomer zoomer) {
                containerView.post(new Runnable() {
                    @Override
                    public void run() {
                        if (containerView.indexOfChild(zoomer) != -1) {
                            containerView.removeView(zoomer);
                            containerView.invalidate();
                        }
                    }
                });
            }
        };
        mView.setVisibilityChangedListener(listener);

        OnTapListener tapListener = new OnTapListener() {
            @Override
            public void onResolveTapDisambiguation(
                    long timeMs, float x, float y, boolean isLongPress) {
                if (mNativeTapDisambiguator == 0) return;
                containerView.requestFocus();
                nativeResolveTapDisambiguation(mNativeTapDisambiguator, timeMs, x, y, isLongPress);
            }
        };
        mView.setOnTapListener(tapListener);
        mNativeTapDisambiguator = nativeInit(mWebContents);
        mInitialized = true;
    }

    private boolean initialized() {
        return mInitialized;
    }

    // ImeEventObserver

    @Override
    public void onImeEvent() {
        hidePopup(true);
    }

    @Override
    public void onNodeAttributeUpdated(boolean editable, boolean password) {}

    @Override
    public void onBeforeSendKeyEvent(KeyEvent event) {}

    /**
     * Returns true if the view is currently being shown (or is animating).
     */
    public boolean isShowing() {
        return mView.isShowing();
    }

    /**
     * Sets the last touch point (on the unzoomed view).
     */
    public void setLastTouch(float x, float y) {
        mView.setLastTouch(x, y);
    }

    /**
     * Show the TapDisambiguator view with given target bounds.
     */
    public void showPopup(Rect rect) {
        mView.show(rect);
    }

    /**
     * Hide the TapDisambiguator view because of some external event such as focus
     * change, JS-originating scroll, etc.
     * @param animation true if hide with animation.
     */
    public void hidePopup(boolean animation) {
        mView.hide(animation);
    }

    /**
     * Called when back button is pressed.
     */
    public void backButtonPressed() {
        mView.backButtonPressed();
    }

    @CalledByNative
    private void destroy() {
        mNativeTapDisambiguator = 0;
    }

    @CalledByNative
    private void showPopup(Rect targetRect, Bitmap zoomedBitmap) {
        mView.setBitmap(zoomedBitmap);
        mView.show(targetRect);
    }

    @CalledByNative
    private void hidePopup() {
        hidePopup(false);
    }

    @CalledByNative
    private static Rect createRect(int x, int y, int right, int bottom) {
        return new Rect(x, y, right, bottom);
    }

    @VisibleForTesting
    void setPopupZoomerForTest(PopupZoomer view) {
        mView = view;
    }

    private native long nativeInit(WebContents webContents);
    private native void nativeResolveTapDisambiguation(
            long nativeTapDisambiguator, long timeMs, float x, float y, boolean isLongPress);
}
