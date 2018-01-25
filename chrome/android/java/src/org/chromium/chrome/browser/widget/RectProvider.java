// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget;

import android.graphics.Rect;
import android.view.View;

/**
 * Provides a {@link Rect} object that represents a position in screen space.
 */
public class RectProvider {
    /**
     * An observer to be notified of changes to the {@Rect} position for the associated
     * {@link View}.
     */
    public interface Observer {
        /**
         * Called when the {@link Rect} location for the {@link View} has changed. {@code newRect}
         * is guaranteed to be non-null.
         */
        void onRectChanged(Rect newRect);

        /** Called when the {@link Rect} for the {@link View} is no longer visible. */
        void onRectHidden();
    }

    /**
     * The {@link Rect} provided by this provider. This is the Rect that will be passed to
     * observers and returned from {@link #getRect()}.
     */
    protected final Rect mRect = new Rect();

    private Observer mObserver;

    /** Creates an instance of a {@link RectProvider}. */
    public RectProvider() {}

    /**
     * Sets the {@link Rect} provided by this provider.
     * @param rect The {@link Rect} to provide.
     */
    public void setRect(Rect rect) {
        mRect.set(rect);
        notifyRectChanged();
    }

    /**
     * Start observing changes to the {@link View}'s position in the window.
     * @param observer The {@link Observer} to be notified of changes.
     */
    public void startObserving(Observer observer) {
        assert mObserver == null || mObserver == observer;

        mObserver = observer;
    }

    /**
     * Stop observing changes to the {@link View}'s position in the window.
     */
    public void stopObserving() {
        mObserver = null;
    }

    /** @return The {@link Rect} that this provider represents. */
    public Rect getRect() {
        return mRect;
    }

    /** Notify the observer that the {@link Rect} changed. */
    protected void notifyRectChanged() {
        if (mObserver != null) mObserver.onRectChanged(mRect);
    }

    /** Notify the observer that the {@link Rect} changed. */
    protected void notifyRectHidden() {
        if (mObserver != null) mObserver.onRectHidden();
    }
}
