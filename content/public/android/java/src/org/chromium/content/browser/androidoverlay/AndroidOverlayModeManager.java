// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.androidoverlay;

import org.chromium.base.ThreadUtils;

import java.util.ArrayList;

/**
 * Singleton class in charge of broadcasting overlay mode changes to listeners.
 * Overlay mode means that we are currently using AndroidOverlays, and that the
 * compositor's surface should support alpha and not be marked as opaque.
 */
public class AndroidOverlayModeManager {
    // The singleton instance.
    private static AndroidOverlayModeManager sInstance;

    private final ArrayList<Listener> mListeners = new ArrayList<>();

    private boolean mInOverlayMode = false;

    /**
     * A listener that gets notified of changes to the usage of AndroidOverlays.
     */
    public interface Listener {
        /**
         * Called whenever we start/stop using AndroidOverlays.
         * @param useOverlayMode Whether the listener should now be in overlay mode.
         */
        public void onOverlayModeChanged(boolean useOverlayMode);
    }

    /**
     * Only access the class via getInstance().
     */
    private AndroidOverlayModeManager() {}

    /**
     * Can only be accessed on the UI thread.
     */
    public static AndroidOverlayModeManager getInstance() {
        ThreadUtils.assertOnUiThread();
        if (sInstance == null) {
            sInstance = new AndroidOverlayModeManager();
        }

        return sInstance;
    }

    /**
     * @param listener The {@link Listener} to be notified of overlay mode changes.
     */
    public void addListener(Listener listener) {
        if (!mListeners.contains(listener)) mListeners.add(listener);
    }

    /**
     * @param listener The {@link Listener} to no longer be notified of overlay mode changes.
     */
    public void removeListener(Listener listener) {
        mListeners.remove(listener);
    }

    /**
     * Set the overlay mode.
     * Notifies listeners via the onOverlayModeChanged() if the overlay mode changed.
     */
    public void setOverlayMode(boolean useOverlayMode) {
        if (mInOverlayMode == useOverlayMode) return;

        mInOverlayMode = useOverlayMode;

        for (Listener listener : mListeners) {
            listener.onOverlayModeChanged(useOverlayMode);
        }
    }
}
