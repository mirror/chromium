// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import org.chromium.base.ObserverList;
import org.chromium.base.ObserverList.RewindableIterator;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content.browser.webcontents.WebContentsImpl;
import org.chromium.content_public.browser.GestureListenerManager;
import org.chromium.content_public.browser.GestureStateListener;

/**
 */
@JNINamespace("content")
public class GestureListenerManagerImpl implements GestureListenerManager {
    private final WebContentsImpl mWebContents;
    private final ObserverList<GestureStateListener> mListeners;
    private final RewindableIterator<GestureStateListener> mIterator;
    private long mNativePtr; // Delete

    public GestureListenerManagerImpl(WebContentsImpl webContents) {
        mWebContents = webContents;
        mListeners = new ObserverList<GestureStateListener>();
        mIterator = mListeners.rewindableIterator();
        mNativePtr = nativeInit(webContents);
    }

    @Override
    public void addListener(GestureStateListener listener) {
        mListeners.addObserver(listener);
    }

    @Override
    public void removeListener(GestureStateListener listener) {
        mListeners.removeObserver(listener);
    }

    public void updateOnDestroyed() {
        for (mIterator.rewind(); mIterator.hasNext();) mIterator.next().onDestroyed();
    }

    public void updateOnTouchDown() {
        for (mIterator.rewind(); mIterator.hasNext();) mIterator.next().onTouchDown();
    }

    public void updateOnWindowFocusChanged(boolean hasFocus) {
        for (mIterator.rewind(); mIterator.hasNext();)
            mIterator.next().onWindowFocusChanged(hasFocus);
    }

    public void updateOnScrollChanged(int offset, int extent) {
        for (mIterator.rewind(); mIterator.hasNext();) {
            mIterator.next().onScrollOffsetOrExtentChanged(offset, extent);
        }
    }

    public void updateOnScrollEnd() {
        for (mIterator.rewind(); mIterator.hasNext();) {
            mIterator.next().onScrollEnded(verticalScrollOffset(), verticalScrollExtent());
        }
    }

    public void updateOnScaleLimitsChanged(float minScale, float maxScale) {
        for (mIterator.rewind(); mIterator.hasNext();) {
            mIterator.next().onScaleLimitsChanged(minScale, maxScale);
        }
    }

    public void updateOnFlingEnd() {
        for (mIterator.rewind(); mIterator.hasNext();) {
            mIterator.next().onFlingEndGesture(verticalScrollOffset(), verticalScrollExtent());
        }
    }

    @CalledByNative
    private void onFlingStartEventConsumed() {
        for (mIterator.rewind(); mIterator.hasNext();) {
            mIterator.next().onFlingStartGesture(verticalScrollOffset(), verticalScrollExtent());
        }
    }

    @CalledByNative
    private void onFlingCancelEventAck() {
        // Do nothing.
    }

    @CalledByNative
    private void onScrollBeginEventAck() {
        for (mIterator.rewind(); mIterator.hasNext();) {
            mIterator.next().onScrollStarted(verticalScrollOffset(), verticalScrollExtent());
        }
    }

    @CalledByNative
    private void onScrollEndEventAck() {
        updateOnScrollEnd();
    }

    @CalledByNative
    private void onScrollUpdateGestureConsumed() {
        for (mIterator.rewind(); mIterator.hasNext();) {
            mIterator.next().onScrollUpdateGestureConsumed();
        }
    }

    @CalledByNative
    private void onPinchBeginEventAck() {
        for (mIterator.rewind(); mIterator.hasNext();) mIterator.next().onPinchStarted();
    }

    @CalledByNative
    private void onPinchEndEventAck() {
        for (mIterator.rewind(); mIterator.hasNext();) mIterator.next().onPinchEnded();
    }

    @CalledByNative
    private void onSingleTapEventAck(boolean consumed) {
        for (mIterator.rewind(); mIterator.hasNext();) {
            mIterator.next().onSingleTap(consumed);
        }
    }

    @CalledByNative
    private void onLongPressAck() {
        for (mIterator.rewind(); mIterator.hasNext();) {
            mIterator.next().onLongPress();
        }
    }

    @CalledByNative
    private void onDestroy() {
        mListeners.clear();
        mNativePtr = 0;
    }

    private int verticalScrollOffset() {
        return mWebContents.getRenderCoordinates().getScrollYPixInt();
    }

    private int verticalScrollExtent() {
        return mWebContents.getRenderCoordinates().getLastFrameViewportHeightPixInt();
    }

    private native long nativeInit(WebContentsImpl webContents);
}
